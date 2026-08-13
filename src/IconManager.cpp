#include "SmartImageProvider.h"
#include "IconManager.h"
#include <QPainter>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QCoreApplication>
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
IconManager* IconManager::m_instance = nullptr;

IconManager* IconManager::instance()
{
    if (!m_instance) m_instance = new IconManager();
    return m_instance;
}

IconManager::IconManager(QObject *parent) : QObject(parent)
{
    loadKeywordsFromJSON();
    loadIcons();
}

// ---------- قراءة البنية الجديدة (languages) ----------
void IconManager::loadKeywordsFromJSON()
{
    QString jsonPath = SmartImageProvider::resourcesPath() + "icon_mappings.json";
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) { loadDefaultKeywords(); return; }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isArray()) { loadDefaultKeywords(); return; }

    keywordMap.clear();
    categoryWeights.clear();

    for (const QJsonValue &val : doc.array()) {
        QJsonObject obj = val.toObject();
        QString cat = obj["category"].toString();
        QJsonObject langs = obj["languages"].toObject();
        for (const QString &lang : langs.keys()) {
            QJsonObject langObj = langs[lang].toObject();
            QStringList words;
            for (const auto &k : langObj["keywords"].toArray()) words << k.toString();
            for (const auto &k : langObj["learned"].toArray()) words << k.toString();
            keywordMap[cat][lang] = words;
            categoryWeights[cat][lang] = langObj["weight"].toDouble(1.0);
        }
    }

    categoryColors = {
        {"politics", "#e74c3c"}, {"sports", "#2ecc71"}, {"health", "#1abc9c"},
        {"economy", "#f39c12"}, {"tech", "#3498db"}, {"military", "#2c3e50"},
        {"environment", "#27ae60"}, {"culture", "#9b59b6"}, {"default", "#95a5a6"}
    };
}

void IconManager::loadDefaultKeywords()
{
    keywordMap["military"]["ar"] = QStringList() << "حرب" << "قصف" << "جيش" << "طائرة" << "صاروخ" << "عسكري";
    keywordMap["military"]["en"] = QStringList() << "war" << "bombing" << "army" << "missile" << "military";
    keywordMap["sports"]["ar"] = QStringList() << "كرة" << "مباراة" << "فيفا" << "كأس" << "دوري";
    keywordMap["sports"]["en"] = QStringList() << "football" << "soccer" << "match" << "fifa" << "cup";
    keywordMap["politics"]["ar"] = QStringList() << "سياسة" << "حكومة" << "وزير" << "برلمان";
    keywordMap["politics"]["en"] = QStringList() << "politics" << "government" << "minister";
    keywordMap["health"]["ar"] = QStringList() << "صحة" << "مستشفى" << "دواء" << "فيروس";
    keywordMap["health"]["en"] = QStringList() << "health" << "hospital" << "medicine" << "virus";
    keywordMap["economy"]["ar"] = QStringList() << "اقتصاد" << "بورصة" << "دولار" << "سوق";
    keywordMap["economy"]["en"] = QStringList() << "economy" << "stock" << "dollar" << "market";
    keywordMap["tech"]["ar"] = QStringList() << "تكنولوجيا" << "ذكاء" << "حاسوب" << "هاتف";
    keywordMap["tech"]["en"] = QStringList() << "technology" << "tech" << "computer" << "phone";
    keywordMap["environment"]["ar"] = QStringList() << "بيئة" << "مناخ" << "طبيعة";
    keywordMap["environment"]["en"] = QStringList() << "environment" << "climate" << "nature";
    keywordMap["culture"]["ar"] = QStringList() << "ثقافة" << "فن" << "كتاب" << "سينما";
    keywordMap["culture"]["en"] = QStringList() << "culture" << "art" << "book" << "cinema";

    for (const QString &category : keywordMap.keys()) {
        for (const QString &lang : keywordMap[category].keys()) {
            if (!categoryWeights[category].contains(lang))
                categoryWeights[category][lang] = 1.0;
        }
    }

    categoryColors = {
        {"politics", "#e74c3c"}, {"sports", "#2ecc71"}, {"health", "#1abc9c"},
        {"economy", "#f39c12"}, {"tech", "#3498db"}, {"military", "#2c3e50"},
        {"environment", "#27ae60"}, {"culture", "#9b59b6"}, {"default", "#95a5a6"}
    };
}

void IconManager::loadIcons() {}

// ---------- التصنيف لكل لغة ----------
QString IconManager::classifyNews(const QString &title, const QString &description, const QString &langCode) const
{
    QString fullText = title + " " + description;
    QStringList stopWords = {
        "في", "عن", "من", "الى", "the", "a", "an", "of", "in", "to", "for", "and", "is", "it", "on", "at", "by", "with", "as", "be"
    };

    auto splitWords = [&](const QString &text) -> QStringList {
        QStringList words;
        for (const QString &part : text.split(QRegularExpression("[\\s،,.;:!?\"'()\\[\\]{}]+"), Qt::SkipEmptyParts)) {
            QString w = part.toLower().trimmed();
            if (!stopWords.contains(w) && w.length() > 1) words << w;
        }
        return words;
    };

    auto extractPhrases = [&](const QStringList &ws) {
        QStringList phrases;
        for (int i = 0; i < ws.size()-1; ++i) phrases << ws[i]+" "+ws[i+1];
        for (int i = 0; i < ws.size()-2; ++i) phrases << ws[i]+" "+ws[i+1]+" "+ws[i+2];
        return phrases;
    };

    QStringList words = splitWords(fullText);
    QStringList phrases = extractPhrases(words);

    QMap<QString, double> scores;

    auto addScores = [&](const QStringList &items, double weight, bool isPhrase = false) {
        for (const QString &item : items) {
            for (auto it = keywordMap.begin(); it != keywordMap.end(); ++it) {
                const QString &cat = it.key();
                if (!it.value().contains(langCode)) continue;
                const QStringList &kws = it.value()[langCode];
                double w = categoryWeights.value(cat).value(langCode, 1.0);
                if (kws.contains(item, Qt::CaseInsensitive)) scores[cat] += weight * w;
                else if (!isPhrase && item.length() >= 4) {
                    for (const QString &kw : kws)
                        if (kw.contains(item, Qt::CaseInsensitive)) { scores[cat] += weight * 0.5 * w; break; }
                }
            }
        }
    };

    addScores(phrases, 3.0, true);
    addScores(splitWords(title), 2.0, false);
    addScores(splitWords(description), 1.0, false);

    if (scores.isEmpty()) return "default";
    QString best = "default"; double max = 0;
    for (auto it = scores.begin(); it != scores.end(); ++it)
        if (it.value() > max) { max = it.value(); best = it.key(); }
    return best;
}

// ---------- getTopCategories لكل لغة ----------
QList<QPair<QString, double>> IconManager::getTopCategories(const QString &title, const QString &description, const QString &langCode) const
{
    QString fullText = title + " " + description;
    QStringList stopWords = {
        "في", "عن", "من", "الى", "the", "a", "an", "of", "in", "to", "for", "and", "is", "it", "on", "at", "by", "with", "as", "be"
    };
    auto splitWords = [&](const QString &text) -> QStringList {
        QStringList words;
        for (const QString &part : text.split(QRegularExpression("[\\s،,.;:!?\"'()\\[\\]{}]+"), Qt::SkipEmptyParts)) {
            QString w = part.toLower().trimmed();
            if (!stopWords.contains(w) && w.length() > 1) words << w;
        }
        return words;
    };
    auto extractPhrases = [&](const QStringList &ws) {
        QStringList phrases;
        for (int i = 0; i < ws.size()-1; ++i) phrases << ws[i]+" "+ws[i+1];
        for (int i = 0; i < ws.size()-2; ++i) phrases << ws[i]+" "+ws[i+1]+" "+ws[i+2];
        return phrases;
    };
    QStringList words = splitWords(fullText);
    QStringList phrases = extractPhrases(words);
    QMap<QString, double> scores;
    auto addScores = [&](const QStringList &items, double weight, bool isPhrase = false) {
        for (const QString &item : items) {
            for (auto it = keywordMap.begin(); it != keywordMap.end(); ++it) {
                const QString &cat = it.key();
                if (!it.value().contains(langCode)) continue;
                const QStringList &kws = it.value()[langCode];
                double w = categoryWeights.value(cat).value(langCode, 1.0);
                if (kws.contains(item, Qt::CaseInsensitive)) scores[cat] += weight * w;
                else if (!isPhrase && item.length() >= 4) {
                    for (const QString &kw : kws)
                        if (kw.contains(item, Qt::CaseInsensitive)) { scores[cat] += weight * 0.5 * w; break; }
                }
            }
        }
    };
    addScores(phrases, 3.0, true);
    addScores(splitWords(title), 2.0, false);
    addScores(splitWords(description), 1.0, false);
    QList<QPair<QString, double>> sorted;
    for (auto it = scores.begin(); it != scores.end(); ++it) sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) { return a.second > b.second; });
    return sorted;
}

bool IconManager::isConfident(const QList<QPair<QString, double>> &scores) const
{
    if (scores.isEmpty()) return false;
    if (scores.size() == 1) return true;
    return (scores[0].second - scores[1].second) > 1.5;
}

QStringList IconManager::getAllCategories() const
{
    return categoryColors.keys();
}

QString IconManager::getCategoryColor(const QString &category) const
{
    return categoryColors.value(category, "#95a5a6");
}

QPixmap IconManager::getIconForNews(const QString &title, const QString &description, const QString &langCode, int size)
{
    QString category = classifyNews(title, description, langCode);
    QColor color = QColor(getCategoryColor(category));
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(2, 2, size - 4, size - 4);
    painter.setPen(QPen(Qt::white, 2));
    painter.setFont(QFont("Segoe UI", size / 3, QFont::Bold));
    QString symbol = category.isEmpty() ? "?" : category.left(1);
    painter.drawText(QRect(0, 0, size, size), Qt::AlignCenter, symbol);
    return pixmap;
}

QPixmap IconManager::getDefaultIcon(int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor("#95a5a6"));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(2, 2, size - 4, size - 4);
    painter.setPen(QPen(Qt::white, 2));
    painter.setFont(QFont("Segoe UI", size / 3));
    painter.drawText(QRect(0, 0, size, size), Qt::AlignCenter, "?");
    return pixmap;
}
