#include "MagazinePage.h"
#include "SmartImageProvider.h"
#include "ResourceManager.h"  // 🔥 تمت الإضافة
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFont>
#include <QPalette>
#include <QPainter>
#include <QPixmap>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDir>
#include <QCryptographicHash>
#include <QFile>
#include <QDebug>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QTimer>
#include <QProgressBar>
#include <QApplication>
#include <QMap>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QSharedPointer>
#include <QPair>
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

static QString getCategoryColor(const QString &category)
{
    static QMap<QString, QString> colors = {
        {"politics", "#e74c3c"},
        {"sports", "#2ecc71"},
        {"health", "#1abc9c"},
        {"economy", "#f39c12"},
        {"tech", "#3498db"},
        {"military", "#2c3e50"},
        {"environment", "#27ae60"},
        {"culture", "#9b59b6"},
        {"default", "#95a5a6"}
    };
    return colors.value(category, "#95a5a6");
}

static QString detectLanguage(const QString &text) {
    for (const QChar &ch : text) {
        if (ch.unicode() >= 0x0600 && ch.unicode() <= 0x06FF) return "ar";
    }
    return "en";
}

MagazinePage::MagazinePage(QWidget *parent)
    : QWidget(parent)
    , loadingCount(0)
    , totalImages(0)
{
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &MagazinePage::onImageLoaded);
    
    imageCache.setMaxCost(30 * 1024 * 1024);
    
    initUI();
    
    showLoadingMessage(tr("📖 Welcome! Your daily magazine will appear here."));
}

MagazinePage::~MagazinePage()
{
    imageCache.clear();
}

void MagazinePage::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    loadingLabel = new QLabel(this);
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLabel->setStyleSheet(R"(
        QLabel {
            padding: 40px;
            font-size: 18px;
            color: #7f8c8d;
            background: #f8f9fa;
        }
    )");
    loadingLabel->setVisible(true);
    mainLayout->addWidget(loadingLabel);

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(R"(
        QScrollArea {
            border: none;
            background-color: #f8f9fa;
        }
        QScrollBar:vertical {
            width: 10px;
            background: #f1f1f1;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical {
            background: #bdc3c7;
            border-radius: 5px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background: #95a5a6;
        }
    )");

    container = new QWidget(scrollArea);
    container->setStyleSheet("background: #f8f9fa;");
    
    gridLayout = new QGridLayout(container);
    gridLayout->setContentsMargins(20, 20, 20, 20);
    gridLayout->setSpacing(25);
    container->setLayout(gridLayout);

    scrollArea->setWidget(container);
    scrollArea->setVisible(false);
    mainLayout->addWidget(scrollArea);
}

void MagazinePage::buildMagazine(const QList<NewsItem> &articles)
{
    clearMagazine();
    
    if (articles.isEmpty()) {
        showLoadingMessage(tr("📭 No news to display in today's magazine."));
        return;
    }

    loadingLabel->setVisible(false);
    scrollArea->setVisible(true);
    
    int columns = (width() > 1200) ? 3 : 2;
    int row = 0, col = 0;
    totalImages = articles.size();
    loadingCount = 0;

    for (const NewsItem &item : articles) {
        createCard(item, row, col);
        col++;
        if (col >= columns) {
            col = 0;
            row++;
        }
    }
}

void MagazinePage::createCard(const NewsItem &item, int row, int col)
{
    QFrame *card = new QFrame(container);
    card->setStyleSheet(R"(
        QFrame {
            background-color: white;
            border-radius: 15px;
            border: 1px solid #e0e0e0;
            padding: 0;
        }
        QFrame:hover {
            border: 2px solid #3498db;
        }
    )");

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    QLabel *imageLabel = new QLabel(card);
    imageLabel->setFixedHeight(200);
    imageLabel->setScaledContents(true);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet(R"(
        QLabel {
            background-color: #ecf0f1;
            border-radius: 15px 15px 0 0;
            color: #bdc3c7;
            font-size: 48px;
        }
    )");
    imageLabel->setText("📷");
    cardLayout->addWidget(imageLabel);

    QWidget *infoWidget = new QWidget(card);
    infoWidget->setStyleSheet("background: white; border-radius: 0 0 15px 15px; padding: 10px;");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoWidget);
    infoLayout->setContentsMargins(12, 10, 12, 12);
    infoLayout->setSpacing(5);

    // --- استخدام اللغة الصحيحة ---
    QString langCode = detectLanguage(item.title + " " + item.description);
    QString category = IconManager::instance()->classifyNews(item.title, item.description, langCode);

    QLabel *categoryLabel = new QLabel(tr(category.toUtf8().constData()), infoWidget);
    categoryLabel->setStyleSheet(QString(R"(
        QLabel {
            font-size: 11px;
            color: white;
            background-color: %1;
            padding: 3px 12px;
            border-radius: 12px;
            font-weight: bold;
            max-width: 100px;
        }
    )").arg(getCategoryColor(category)));
    categoryLabel->setAlignment(Qt::AlignCenter);
    infoLayout->addWidget(categoryLabel, 0, Qt::AlignRight);

    QLabel *titleLabel = new QLabel(item.title, infoWidget);
    titleLabel->setWordWrap(true);
    titleLabel->setStyleSheet(R"(
        QLabel {
            font-size: 18px;
            font-weight: bold;
            color: #1a1a1a;
        }
    )");
    titleLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    infoLayout->addWidget(titleLabel);

    QLabel *descLabel = new QLabel(item.description, infoWidget);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet(R"(
        QLabel {
            font-size: 14px;
            color: #2c2c2c;
            padding: 0px;
        }
    )");
    descLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    descLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    infoLayout->addWidget(descLabel);

    QLabel *sourceLabel = new QLabel(infoWidget);
    sourceLabel->setTextFormat(Qt::RichText);
    QString sourceLink = QString("<a href=\"%1\" style=\"color: #3498db; text-decoration: none;\">📌 %2</a> <span style=\"color: #95a5a6; font-size: 11px;\">| 🕒 %3</span>")
                         .arg(item.link)
                         .arg(item.source)
                         .arg(item.pubDate);
    sourceLabel->setText(sourceLink);
    sourceLabel->setOpenExternalLinks(true);
    sourceLabel->setStyleSheet("font-size: 12px; font-weight: bold;");
    infoLayout->addWidget(sourceLabel);

    cardLayout->addWidget(infoWidget);

    QPixmap smartImage = SmartImageProvider::instance()->getImageForNews(item, 400, 200, langCode);
    imageLabel->setPixmap(smartImage);
    imageLabel->setText("");
    loadingCount++;
    updateLoadingStatus();

    // --- أزرار التصحيح الذكية ---
    QList<QPair<QString, double>> topCategories = IconManager::instance()->getTopCategories(item.title, item.description, langCode);
    bool lowConfidence = !IconManager::instance()->isConfident(topCategories);
    bool isDefault = (category == "default");

    if (lowConfidence || isDefault) {
        QStringList allCats = IconManager::instance()->getAllCategories();
        allCats.removeAll("default");

        QStringList catsToShow;
        if (!isDefault && !topCategories.isEmpty()) {
            for (const auto &pair : topCategories) {
                if (!catsToShow.contains(pair.first) && pair.first != "default")
                    catsToShow.append(pair.first);
            }
        }
        if (catsToShow.isEmpty()) {
            catsToShow = allCats;
        }

        QWidget *altBar = new QWidget(card);
        altBar->setStyleSheet("background: transparent;");   
        altBar->setFixedWidth(110); // عرض ثابت مناسب للأزرار
        QVBoxLayout *altLayout = new QVBoxLayout(altBar);
        altLayout->setContentsMargins(6, 6, 6, 6);
        altLayout->setSpacing(6);
        altLayout->addStretch(); // يدفع الأزرار للأعلى

        for (const QString &altCat : catsToShow) {
            if (altCat == category && !isDefault) continue;

            QPushButton *btn = new QPushButton();
            btn->setMinimumWidth(90);
            btn->setFixedHeight(28);
            QString color = getCategoryColor(altCat);
            btn->setStyleSheet(QString(R"(
                QPushButton {
                    background-color: %1;
                    color: white;
                    border-radius: 14px;
                    border: none;
                    font-size: 11px;
                    font-weight: bold;
                    padding: 2px 12px;
                }
                QPushButton:hover { border: 2px solid white; }
            )").arg(color));
            btn->setText(tr(altCat.toUtf8().constData()));

            connect(btn, &QPushButton::clicked, this, [this, item, altCat, imageLabel, categoryLabel, altBar, langCode]() {
                // 1. تعلم من التصحيح
                SmartImageProvider::instance()->learnFromNews(item, altCat, langCode);

                // 2. ✅ أعد تحميل IconManager من الملف المُحدّث
                IconManager::instance()->loadKeywordsFromJSON();

                // 3. أعد توليد الصورة بالتصنيف الجديد
                QPixmap newImage = SmartImageProvider::instance()->getImageForNews(item, 400, 200, langCode);
                imageLabel->setPixmap(newImage);
                imageLabel->repaint();

                // 4. تحديث النص واللون
                categoryLabel->setText(tr(altCat.toUtf8().constData()));
                categoryLabel->setStyleSheet(QString(R"(
                    QLabel {
                        font-size: 11px; color: white; background-color: %1;
                        padding: 3px 12px; border-radius: 12px; font-weight: bold;
                        max-width: 100px;
                    }
                )").arg(getCategoryColor(altCat)));

                // 5. إخفاء الأزرار
                altBar->deleteLater();
            });

            altLayout->addWidget(btn);
        }
        altLayout->addStretch(); // يدفع الأزرار للأسفل

        // وضع الشريط على يمين الصورة
        altBar->setParent(imageLabel);
        altBar->setGeometry(imageLabel->width() - 110, 0, 110, 200);
        altBar->show();
    }

    gridLayout->addWidget(card, row, col);
    addCardAnimation(card, (row * 2 + col) * 100);
}

void MagazinePage::addCardAnimation(QWidget *card, int delay)
{
    QPropertyAnimation *animation = new QPropertyAnimation(card, "opacity");
    animation->setDuration(500);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    
    QTimer::singleShot(delay, [animation]() { animation->start(); });
    connect(animation, &QPropertyAnimation::finished, animation, &QPropertyAnimation::deleteLater);
}

void MagazinePage::loadImage(const QString &url, QLabel *targetLabel)
{
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", "Mozilla/5.0");
    request.setTransferTimeout(10000);
    QNetworkReply *reply = networkManager->get(request);
    pendingImages[reply] = targetLabel;
}

void MagazinePage::onImageLoaded(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "فشل تحميل الصورة:" << reply->errorString();
        reply->deleteLater();
        pendingImages.remove(reply);
        loadingCount++;
        updateLoadingStatus();
        return;
    }
    QLabel *targetLabel = pendingImages.value(reply);
    if (targetLabel) {
        QByteArray data = reply->readAll();
        QPixmap pixmap;
        if (pixmap.loadFromData(data)) {
            QString url = reply->url().toString();
            saveImageToCache(url, data);
            QPixmap scaled = pixmap.scaled(targetLabel->width(), targetLabel->height(),
                                           Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            targetLabel->setPixmap(scaled);
            targetLabel->setText("");
        }
    }
    reply->deleteLater();
    pendingImages.remove(reply);
    loadingCount++;
    updateLoadingStatus();
}

void MagazinePage::updateLoadingStatus()
{
    if (totalImages > 0) {
        loadingLabel->setText(tr("🔄 Loading images... %1/%2").arg(loadingCount).arg(totalImages));
        if (loadingCount >= totalImages) {
            loadingLabel->setVisible(false);
            loadingLabel->setText(tr("✅ All images loaded!"));
            QTimer::singleShot(500, [this]() { loadingLabel->setVisible(false); });
        }
    }
}

// 🔥 تم التعديل لاستخدام ResourceManager للكاش
bool MagazinePage::loadImageFromCache(const QString &url, QLabel *targetLabel)
{
    QString key = getCacheKey(url);
    QPixmap *cached = imageCache.object(key);
    if (cached && !cached->isNull()) {
        QPixmap scaled = cached->scaled(targetLabel->width(), targetLabel->height(),
                                        Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        targetLabel->setPixmap(scaled);
        targetLabel->setText("");
        return true;
    }
    
    // استخدام ResourceManager للحصول على مسار الكاش الصحيح
    QString cachePath = ResourceManager::getCachePath() + "/images/";
    QString filePath = cachePath + key + ".jpg";
    
    if (QFile::exists(filePath)) {
        QPixmap pixmap;
        if (pixmap.load(filePath)) {
            QPixmap *cachedPixmap = new QPixmap(pixmap);
            int imageSize = pixmap.width() * pixmap.height() * (pixmap.depth() / 8);
            imageCache.insert(key, cachedPixmap, imageSize);
            QPixmap scaled = pixmap.scaled(targetLabel->width(), targetLabel->height(),
                                           Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            targetLabel->setPixmap(scaled);
            targetLabel->setText("");
            return true;
        }
    }
    return false;
}

// 🔥 تم التعديل لاستخدام ResourceManager للكاش
void MagazinePage::saveImageToCache(const QString &url, const QByteArray &data)
{
    QString key = getCacheKey(url);
    QPixmap *pixmap = new QPixmap();
    if (pixmap->loadFromData(data)) {
        int imageSize = pixmap->width() * pixmap->height() * (pixmap->depth() / 8);
        imageCache.insert(key, pixmap, imageSize);
    }
    
    // استخدام ResourceManager للحصول على مسار الكاش الصحيح
    QString cachePath = ResourceManager::getCachePath() + "/images/";
    QDir cacheDir(cachePath);
    if (!cacheDir.exists()) {
        cacheDir.mkpath(".");
    }
    QString filePath = cacheDir.filePath(key + ".jpg");
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
    }
}

QString MagazinePage::getCacheKey(const QString &url) const
{
    return QString(QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Md5).toHex());
}

QString MagazinePage::getCategoryForNews(const NewsItem &item) const
{
    QString langCode = detectLanguage(item.title + " " + item.description);
    return IconManager::instance()->classifyNews(item.title, item.description, langCode);
}

void MagazinePage::clearMagazine()
{
    while (gridLayout->count()) {
        QLayoutItem *child = gridLayout->takeAt(0);
        if (child->widget()) delete child->widget();
        delete child;
    }
    pendingImages.clear();
    loadingCount = 0;
    totalImages = 0;
    scrollArea->setVisible(false);
    loadingLabel->setVisible(true);
}

void MagazinePage::showLoadingMessage(const QString &message)
{
    loadingLabel->setText(message);
    loadingLabel->setVisible(true);
    scrollArea->setVisible(false);
}

void MagazinePage::setLoading(bool loading)
{
    loadingLabel->setVisible(loading);
    scrollArea->setVisible(!loading);
}

void MagazinePage::applyCardStyle(QFrame *card)
{
    // تم التطبيق مباشرة في createCard
}

void MagazinePage::onCardAnimationFinished()
{
    // تم التطبيق مباشرة في addCardAnimation
}
