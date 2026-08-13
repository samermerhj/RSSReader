#ifndef MAGAZINEPAGE_H
#define MAGAZINEPAGE_H

#include <QWidget>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QCache>
#include <QPixmap>
#include <QParallelAnimationGroup>
#include <QGridLayout>      // ✅ تمت الإضافة
#include <QLabel>           // ✅ تمت الإضافة
#include <QScrollArea>      // ✅ تمت الإضافة
#include <QFrame>           // ✅ تمت الإضافة
#include <QMap>             // ✅ تمت الإضافة
#include "DatabaseManager.h"
#include "IconManager.h"
#include "ImageDatabase.h"

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
class MagazinePage : public QWidget
{
    Q_OBJECT
public:
    explicit MagazinePage(QWidget *parent = nullptr);
    ~MagazinePage();

    // بناء المجلة من قائمة الأخبار المجمعة
    void buildMagazine(const QList<NewsItem> &articles);
    
    // مسح المجلة
    void clearMagazine();
    
    // إظهار/إخفاء شريط التحميل
    void setLoading(bool loading);

private slots:
    void onImageLoaded(QNetworkReply *reply);
    void onCardAnimationFinished();

private:
    // ------ عناصر الواجهة ------
    QGridLayout *gridLayout;
    QLabel *loadingLabel;
    QWidget *container;
    QScrollArea *scrollArea;
    
    // ------ تحميل الصور ------
    QNetworkAccessManager *networkManager;
    QCache<QString, QPixmap> imageCache;
    QMap<QNetworkReply*, QLabel*> pendingImages;
    int loadingCount;
    int totalImages;
    
    // ------ دوال مساعدة ------
    void createCard(const NewsItem &item, int row, int col);
    void loadImage(const QString &url, QLabel *targetLabel);
    QString getCacheKey(const QString &url) const;
    void saveImageToCache(const QString &url, const QByteArray &data);
    bool loadImageFromCache(const QString &url, QLabel *targetLabel);
    QString getCategoryForNews(const NewsItem &item) const;
    void showLoadingMessage(const QString &message);
    
    // ------ التصميم ------
    void initUI();
    void applyCardStyle(QFrame *card);
    void addCardAnimation(QWidget *card, int delay);
    void updateLoadingStatus();
};

#endif // MAGAZINEPAGE_H
