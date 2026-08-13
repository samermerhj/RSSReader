#include "SmartImageProvider.h"
#include "IconManager.h"
#include <QPainter>
#include <QSvgRenderer>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QCoreApplication>
#include <QDebug>
#include <QtMath>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QRegularExpression>
/**
 * Copyright (C) 2026 Samer Merhj <mjosak7@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
SmartImageProvider* SmartImageProvider::m_instance = nullptr;

SmartImageProvider* SmartImageProvider::instance()
{
    if (!m_instance) m_instance = new SmartImageProvider();
    return m_instance;
}

SmartImageProvider::SmartImageProvider(QObject *parent) : QObject(parent)
{
    initCategoryColors();
    loadMappingsFromJson();
}

QString SmartImageProvider::resourcesPath()
{
    QString path = QCoreApplication::applicationDirPath() + "/resources/";
    if (QDir(path).exists()) return path;
    path = "/usr/share/RSSReader/resources/";
    if (QDir(path).exists()) return path;
    path = QDir::homePath() + "/.local/share/RSSReader/resources/";
    if (QDir(path).exists()) return path;
    QString fallback = QCoreApplication::applicationDirPath() + "/resources/";
    qWarning() << "⚠️ resourcesPath: لم يتم العثور على مجلد resources، سيتم استخدام المسار الاحتياطي:" << fallback;
    return fallback;
}

void SmartImageProvider::loadMappingsFromJson()
{
    QByteArray data; bool loaded = false;
    QString jsonPath = resourcesPath() + "icon_mappings.json";
    QFile file(jsonPath);
    if (file.open(QIODevice::ReadOnly)) { data = file.readAll(); file.close(); loaded = true; }
    if (!loaded) {
        QFile resourceFile(":/resources/icon_mappings.json");
        if (resourceFile.open(QIODevice::ReadOnly)) { data = resourceFile.readAll(); resourceFile.close(); loaded = true; }
    }
    if (!loaded) {
        qWarning() << "SmartImageProvider: لم يتم العثور على icon_mappings.json. سيتم استخدام قائمة افتراضية.";
        // القائمة الافتراضية القديمة لا تزال تعمل كحالة طوارئ
        allKeywords["oil"] = QStringList() << "نفط" << "بترول" << "غاز" << "طاقة" << "oil" << "petroleum" << "gas" << "energy";
        allKeywords["football"] = QStringList() << "كرة" << "مباراة" << "فيفا" << "كأس" << "دوري" << "football" << "soccer" << "match";
        allKeywords["war"] = QStringList() << "حرب" << "قصف" << "جيش" << "طائرة" << "صاروخ" << "عسكري" << "war" << "military" << "army";
        allKeywords["health"] = QStringList() << "صحة" << "مستشفى" << "دواء" << "فيروس" << "وباء" << "طبيب" << "health" << "hospital" << "medicine";
        allKeywords["tech"] = QStringList() << "تكنولوجيا" << "ذكاء" << "حاسوب" << "هاتف" << "برمجة" << "tech" << "computer" << "software";
        allKeywords["economy"] = QStringList() << "اقتصاد" << "بورصة" << "دولار" << "سوق" << "أرباح" << "مال" << "economy" << "stock" << "dollar";
        allKeywords["environment"] = QStringList() << "بيئة" << "مناخ" << "طبيعة" << "environment" << "climate" << "nature";
        allKeywords["culture"] = QStringList() << "ثقافة" << "فن" << "موسيقى" << "culture" << "art" << "music";
        allKeywords["politics"] = QStringList() << "سياسة" << "حكومة" << "وزير" << "politics" << "government" << "minister";
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return;
    QJsonArray array = doc.array();
    for (const QJsonValue &val : array) {
        QJsonObject obj = val.toObject();
        QString iconName = obj.value("icon").toString();
        QStringList keywords;
        // قراءة البنية الجديدة
        QJsonObject langs = obj.value("languages").toObject();
        for (const QString &lang : langs.keys()) {
            QJsonObject langObj = langs[lang].toObject();
            for (const auto &k : langObj["keywords"].toArray()) keywords.append(k.toString());
            for (const auto &k : langObj["learned"].toArray()) keywords.append(k.toString());
        }
        if (allKeywords.contains(iconName)) allKeywords[iconName].append(keywords);
        else allKeywords[iconName] = keywords;
        allKeywords[iconName].removeDuplicates();
    }
    qDebug() << "✅ SmartImageProvider: تم تحميل" << allKeywords.size() << "فئة من الأيقونات.";
}

void SmartImageProvider::initCategoryColors()
{
    categoryColors = {
        {"politics", QColor("#e74c3c")}, {"sports", QColor("#2ecc71")}, {"health", QColor("#1abc9c")},
        {"economy", QColor("#f39c12")}, {"tech", QColor("#3498db")}, {"military", QColor("#2c3e50")},
        {"environment", QColor("#27ae60")}, {"culture", QColor("#9b59b6")}, {"default", QColor("#95a5a6")}
    };
}

QStringList SmartImageProvider::getTopIcons(const QString &text, int maxCount) const
{
    QMap<QString, int> scores;
    QString lowerText = text.toLower();
    for (auto it = allKeywords.begin(); it != allKeywords.end(); ++it) {
        int count = 0;
        for (const QString &keyword : it.value()) {
            count += lowerText.count(keyword.toLower());
        }
        if (count > 0) scores[it.key()] = count;
    }
    QList<QPair<QString, int>> sorted;
    for (auto it = scores.begin(); it != scores.end(); ++it) sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end(), [](const QPair<QString,int> &a, const QPair<QString,int> &b) {
        if (a.second == b.second) {
            QStringList priority = {"war", "oil", "health", "tech", "economy", "football", "environment", "culture"};
            int idxA = priority.indexOf(a.first);
            int idxB = priority.indexOf(b.first);
            if (idxA == -1) idxA = 999;
            if (idxB == -1) idxB = 999;
            return idxA < idxB;
        }
        return a.second > b.second;
    });
    QStringList topIcons;
    for (int i = 0; i < sorted.size() && i < maxCount; ++i) topIcons.append(sorted[i].first);
    return topIcons;
}

bool SmartImageProvider::tryLoadSvg(const QString &iconName, QPixmap &outPixmap, int width, int height) const
{
    QString resourcePath = ":/resources/images/news_icons/" + iconName + ".svg";
    if (QFile::exists(resourcePath)) {
        QSvgRenderer renderer(resourcePath);
        if (renderer.isValid()) {
            outPixmap = QPixmap(width, height);
            outPixmap.fill(Qt::transparent);
            QPainter painter(&outPixmap);
            renderer.render(&painter);
            painter.end();
            return true;
        }
    }
    QString filePath = SmartImageProvider::resourcesPath() + "images/news_icons/" + iconName + ".svg";
    if (QFile::exists(filePath)) {
        QSvgRenderer renderer(filePath);
        if (renderer.isValid()) {
            outPixmap = QPixmap(width, height);
            outPixmap.fill(Qt::transparent);
            QPainter painter(&outPixmap);
            renderer.render(&painter);
            painter.end();
            return true;
        }
    }
    return false;
}

bool SmartImageProvider::loadBackgroundSvg(const QString &category, QPixmap &outPixmap, int width, int height) const
{
    QMap<QString, QString> bgFiles = {
        {"politics", "political"}, {"military", "military"}, {"sports", "sports"},
        {"health", "health"}, {"tech", "tech"}, {"economy", "economy"},
        {"environment", "environment"}, {"culture", "culture"}
    };
    QString fileName = bgFiles.value(category, "default");
    QString resourcePath = ":/resources/images/backgrounds/" + fileName + ".svg";
    if (QFile::exists(resourcePath)) {
        QSvgRenderer renderer(resourcePath);
        if (renderer.isValid()) {
            outPixmap = QPixmap(width, height);
            outPixmap.fill(Qt::transparent);
            QPainter painter(&outPixmap);
            renderer.render(&painter);
            painter.end();
            return true;
        }
    }
    QString filePath = SmartImageProvider::resourcesPath() + "images/backgrounds/" + fileName + ".svg";
    if (QFile::exists(filePath)) {
        QSvgRenderer renderer(filePath);
        if (renderer.isValid()) {
            outPixmap = QPixmap(width, height);
            outPixmap.fill(Qt::transparent);
            QPainter painter(&outPixmap);
            renderer.render(&painter);
            painter.end();
            return true;
        }
    }
    return false;
}

// ---------- دالة التعلم من التصحيح (متوافقة مع البنية الجديدة) ----------
void SmartImageProvider::learnFromNews(const NewsItem &item, const QString &correctCategory, const QString &langCode)
{
    QString fullText = item.title + " " + item.description;
    QStringList allWords = fullText.split(QRegularExpression("[\\s،,.;:!?\"'()\\[\\]{}]+"), Qt::SkipEmptyParts);
    QStringList words;
    for (const QString &word : allWords) {
        QString w = word.trimmed();
        if (w.length() < 3) continue;
        words.append(w);
    }
    words.removeDuplicates();

    QString jsonPath = resourcesPath() + "icon_mappings.json";
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) return;
    QByteArray data = file.readAll(); file.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return;

    QJsonArray array = doc.array();
    bool found = false;
    for (int i = 0; i < array.size(); ++i) {
        QJsonObject obj = array[i].toObject();
        if (obj["category"].toString() == correctCategory) {
            found = true;
            QJsonObject langs = obj["languages"].toObject();
            QJsonObject langObj = langs[langCode].toObject();

            QJsonArray learnedArr = langObj["learned"].toArray();
            for (const QString &w : words) {
                if (!learnedArr.contains(w))
                    learnedArr.append(w);
            }
            langObj["learned"] = learnedArr;

            double weight = langObj["weight"].toDouble(1.0);
            langObj["weight"] = weight + 0.1;

            langs[langCode] = langObj;
            obj["languages"] = langs;
            array[i] = obj;
            break;
        }
    }
    if (!found) {
        QJsonObject newObj;
        newObj["icon"] = correctCategory;
        newObj["category"] = correctCategory;
        QJsonObject langs;
        QJsonObject langObj;
        langObj["keywords"] = QJsonArray();
        QJsonArray learnedArr;
        for (const QString &w : words) learnedArr.append(w);
        langObj["learned"] = learnedArr;
        langObj["weight"] = 1.1;
        langs[langCode] = langObj;
        newObj["languages"] = langs;
        array.append(newObj);
    }
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
        file.close();
        qDebug() << "✅ SmartImageProvider: تم التعلم للغة" << langCode;
    }
    loadMappingsFromJson();
}

void SmartImageProvider::drawColoredCircle(QPainter &p, const QRect &r, const QColor &color, const QString &letter)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(color);
    p.setPen(Qt::NoPen);
    int diameter = qMin(r.width(), r.height()) - 10;
    int x = r.center().x() - diameter / 2;
    int y = r.center().y() - diameter / 2;
    p.drawEllipse(x, y, diameter, diameter);
    p.setPen(Qt::white);
    QFont font("Arial", diameter / 2, QFont::Bold);
    p.setFont(font);
    p.drawText(QRect(x, y, diameter, diameter), Qt::AlignCenter, letter);
    p.restore();
}

QPixmap SmartImageProvider::getImageForNews(const NewsItem &item, int width, int height, const QString &langCode)
{
    QString category = IconManager::instance()->classifyNews(item.title, item.description, langCode);
    QString iconName;
    if (category == "politics") iconName = "politics";
    else if (category == "sports") iconName = "football";
    else if (category == "health") iconName = "health";
    else if (category == "economy") iconName = "economy";
    else if (category == "tech") iconName = "tech";
    else if (category == "military") iconName = "war";
    else if (category == "environment") iconName = "environment";
    else if (category == "culture") iconName = "culture";
    else iconName = "default";

    QPixmap background(width, height);
    if (!loadBackgroundSvg(category, background, width, height)) {
        QPainter bgPainter(&background);
        drawCategoryBackground(bgPainter, category, width, height);
        bgPainter.end();
    }

    QPixmap finalImage = background;
    QPainter finalPainter(&finalImage);
    finalPainter.setRenderHint(QPainter::Antialiasing);

    QPixmap mainPixmap;
    if (!tryLoadSvg(iconName, mainPixmap, width/2, height/2)) {
        mainPixmap = QPixmap(width/2, height/2);
        mainPixmap.fill(Qt::transparent);
        QPainter mp(&mainPixmap);
        mp.setRenderHint(QPainter::Antialiasing);
        QColor catColor = categoryColors.value(category, QColor("#95a5a6"));
        QString letter = category.left(1).toUpper();
        if (category == "default") letter = "?";
        drawColoredCircle(mp, mainPixmap.rect(), catColor, letter);
        mp.end();
    }
    int mainX = (width - mainPixmap.width()) / 2;
    int mainY = (height - mainPixmap.height()) / 2 - 10;
    finalPainter.drawPixmap(mainX, mainY, mainPixmap);

    finalPainter.setPen(Qt::white);
    QFont titleFont; titleFont.setBold(true); titleFont.setPixelSize(12);
    finalPainter.setFont(titleFont);
    QRect textRect(10, height - 30, width - 20, 25);
    finalPainter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, item.title);
    finalPainter.end();
    return finalImage;
}

// ==================== دوال رسم الأيقونات (يدوياً) ====================

void SmartImageProvider::drawIconForName(const QString &name, QPainter &painter, const QRect &rect)
{
    if (name == "oil") drawOilIcon(painter, rect);
    else if (name == "football") drawFootballIcon(painter, rect);
    else if (name == "war") drawWarIcon(painter, rect);
    else if (name == "health") drawHealthIcon(painter, rect);
    else if (name == "tech") drawTechIcon(painter, rect);
    else if (name == "economy") drawEconomyIcon(painter, rect);
    else if (name == "culture") drawCultureIcon(painter, rect);
    else if (name == "environment") drawEnvironmentIcon(painter, rect);
    else if (name == "politics") drawPoliticsIcon(painter, rect);
    else drawDefaultIcon(painter, rect);
}

void SmartImageProvider::drawCategoryBackground(QPainter &painter, const QString &category, int width, int height)
{
    QColor base = categoryColors.value(category, QColor("#95a5a6"));
    painter.setRenderHint(QPainter::Antialiasing);
    if (category == "politics") {
        QLinearGradient grad(0, 0, 0, height); grad.setColorAt(0, QColor("#1a1a3e")); grad.setColorAt(1, QColor("#2c3e50"));
        painter.fillRect(0, 0, width, height, grad);
        painter.setPen(QPen(QColor(255,255,255,20), 1));
        for (int y = 0; y < height; y += 15) painter.drawLine(0, y, width, y);
    } else if (category == "military") {
        QLinearGradient grad(0, 0, 0, height); grad.setColorAt(0, QColor("#4a4a4a")); grad.setColorAt(1, QColor("#2c2c1a"));
        painter.fillRect(0, 0, width, height, grad); painter.setPen(Qt::NoPen);
        for (int i = 0; i < 5; ++i) {
            painter.setBrush(QColor(255,255,255,20));
            painter.drawEllipse(QRandomGenerator::global()->bounded(width), QRandomGenerator::global()->bounded(height),
                                QRandomGenerator::global()->bounded(80)+40, QRandomGenerator::global()->bounded(80)+40);
        }
    } else if (category == "economy") {
        QLinearGradient grad(0, 0, 0, height); grad.setColorAt(0, QColor("#1e3c1e")); grad.setColorAt(1, QColor("#b8860b"));
        painter.fillRect(0, 0, width, height, grad);
        painter.setPen(QPen(QColor(255,215,0,40), 1));
        for (int x = 0; x < width; x += 30) painter.drawLine(x, 0, x, height);
        for (int y = 0; y < height; y += 30) painter.drawLine(0, y, width, y);
    } else if (category == "sports") {
        QLinearGradient grad(0, 0, width, 0); grad.setColorAt(0, QColor("#2e7d32")); grad.setColorAt(1, QColor("#4caf50"));
        painter.fillRect(0, 0, width, height, grad);
        painter.setPen(QPen(QColor(255,255,255,30), 2));
        for (int i = -height; i < width+height; i += 40) painter.drawLine(i, 0, i+height, height);
    } else if (category == "health") {
        QLinearGradient grad(0, 0, 0, height); grad.setColorAt(0, Qt::white); grad.setColorAt(1, QColor("#e3f2fd"));
        painter.fillRect(0, 0, width, height, grad);
        painter.setPen(QPen(QColor(255,0,0,20), 8));
        painter.drawLine(width/2, height/2-50, width/2, height/2+50);
        painter.drawLine(width/2-50, height/2, width/2+50, height/2);
    } else if (category == "tech") {
        QLinearGradient grad(0, 0, 0, height); grad.setColorAt(0, QColor("#0a192f")); grad.setColorAt(1, Qt::black);
        painter.fillRect(0, 0, width, height, grad);
        painter.setPen(QPen(QColor(0,255,255,40), 1));
        for (int x = 0; x < width; x += 20) painter.drawLine(x, 0, x, height);
        for (int y = 0; y < height; y += 20) painter.drawLine(0, y, width, y);
    } else if (category == "environment") {
        QLinearGradient grad(0, 0, 0, height); grad.setColorAt(0, QColor("#87CEEB")); grad.setColorAt(1, QColor("#2e8b57"));
        painter.fillRect(0, 0, width, height, grad);
        painter.setPen(QPen(QColor(255,255,255,40), 2));
        for (int y = 0; y < height; y += 25) {
            QPainterPath path; path.moveTo(0, y);
            for (int x = 0; x <= width; x += 20) path.lineTo(x, y + 8*sin(x/30.0 + y/20.0));
            painter.drawPath(path);
        }
    } else if (category == "culture") {
        QLinearGradient grad(0, 0, 0, height); grad.setColorAt(0, QColor("#4a148c")); grad.setColorAt(1, QColor("#f48fb1"));
        painter.fillRect(0, 0, width, height, grad);
        painter.setPen(QPen(QColor(255,255,255,25), 2));
        for (int i = 0; i < 8; ++i)
            painter.drawArc(QRandomGenerator::global()->bounded(width), QRandomGenerator::global()->bounded(height),
                            100, 100, QRandomGenerator::global()->bounded(360), QRandomGenerator::global()->bounded(180));
    } else {
        QLinearGradient grad(0, 0, 0, height); grad.setColorAt(0, base.lighter(150)); grad.setColorAt(1, base.darker(150));
        painter.fillRect(0, 0, width, height, grad);
    }
}

void SmartImageProvider::drawOilIcon(QPainter &p, const QRect &r) {
    p.setBrush(QColor("#4a4a4a")); p.setPen(Qt::NoPen);
    p.drawRoundedRect(r.adjusted(10,20,-10,-15),10,10);
    p.setBrush(QColor("#2c2c2c")); p.drawRect(r.center().x()-15, r.top()+5, 30, 15);
}
void SmartImageProvider::drawFootballIcon(QPainter &p, const QRect &r) {
    p.setBrush(Qt::white); p.setPen(QPen(Qt::black,2));
    int rad = qMin(r.width(),r.height())/2-10;
    p.drawEllipse(r.center(), rad, rad);
    p.drawLine(r.center(), r.center()+QPoint(20,-20)); p.drawLine(r.center(), r.center()+QPoint(-20,-20));
}
void SmartImageProvider::drawWarIcon(QPainter &p, const QRect &r) {
    p.setBrush(QColor("#5d6d7e")); p.setPen(Qt::NoPen);
    QPointF body[] = {QPointF(r.left()+20, r.center().y()), QPointF(r.right()-20, r.center().y()), QPointF(r.right()-40, r.top()+20)};
    p.drawPolygon(body,3);
}
void SmartImageProvider::drawHealthIcon(QPainter &p, const QRect &r) {
    p.setPen(QPen(Qt::red,10));
    p.drawLine(r.center().x(), r.top()+20, r.center().x(), r.bottom()-20);
    p.drawLine(r.left()+20, r.center().y(), r.right()-20, r.center().y());
}
void SmartImageProvider::drawTechIcon(QPainter &p, const QRect &r) {
    p.setBrush(QColor("#2c3e50")); p.drawRoundedRect(r.adjusted(15,15,-15,-15),8,8);
    p.setBrush(QColor("#3498db")); p.drawRoundedRect(r.adjusted(25,25,-25,-25),4,4);
}
void SmartImageProvider::drawEconomyIcon(QPainter &p, const QRect &r) {
    p.setPen(QPen(QColor("#27ae60"),4));
    p.drawLine(r.left()+10, r.bottom()-10, r.right()-10, r.bottom()-10);
    p.drawLine(r.left()+10, r.bottom()-10, r.left()+10, r.top()+10);
    p.setPen(QPen(QColor("#f1c40f"),3));
    p.drawLine(r.left()+15, r.center().y(), r.center().x(), r.top()+20);
    p.drawLine(r.center().x(), r.top()+20, r.right()-15, r.center().y()-10);
}
void SmartImageProvider::drawCultureIcon(QPainter &p, const QRect &r) {
    p.setPen(QPen(QColor("#8e44ad"),4));
    p.drawLine(r.left()+15, r.top()+10, r.right()-15, r.top()+10);
    for(int i=0;i<5;++i) p.drawLine(r.left()+20+i*15, r.top()+10, r.left()+20+i*15, r.bottom()-10);
}
void SmartImageProvider::drawEnvironmentIcon(QPainter &p, const QRect &r) {
    p.setBrush(QColor("#27ae60")); p.setPen(Qt::NoPen);
    p.drawEllipse(r.center().x()-20, r.top()+15, 40, 40);
    p.setBrush(QColor("#8B4513")); p.drawRect(r.center().x()-5, r.center().y()+10, 10, 40);
}
void SmartImageProvider::drawDefaultIcon(QPainter &p, const QRect &r) {
    p.setPen(QPen(Qt::white,3));
    p.drawEllipse(r.center(), qMin(r.width(),r.height())/2-15, qMin(r.width(),r.height())/2-15);
    p.drawText(r, Qt::AlignCenter, "?");
}
void SmartImageProvider::drawPoliticsIcon(QPainter &p, const QRect &r) {
    p.setBrush(QColor("#e74c3c")); p.setPen(Qt::NoPen);
    p.drawRect(r.left()+10, r.top()+20, r.width()-20, r.height()-30);
    p.drawEllipse(r.center().x()-20, r.top()+5, 40, 25);
    p.setBrush(QColor("#c0392b"));
    for (int i = 0; i < 4; ++i) { int x = r.left()+15 + i*15; p.drawRect(x, r.top()+20, 5, r.height()-30); }
}
