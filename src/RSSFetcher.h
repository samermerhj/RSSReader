#ifndef RSSFETCHER_H
#define RSSFETCHER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QList>
#include <QPair>
#include "DatabaseManager.h"

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
class RSSFetcher : public QObject
{
    Q_OBJECT
public:
    explicit RSSFetcher(DatabaseManager *dbManager, QObject *parent = nullptr);

    // جلب مصدر واحد
    void fetchFeed(const QString &url, const QString &sourceName);
    
    // جلب جميع المصادر (يتم إرسال إشارة allFeedsFetched عند الانتهاء)
    void fetchAllFeeds(const QList<QPair<QString, QString>> &sources);

signals:
    // يتم إرسالها عند جلب مصدر واحد بنجاح
    void feedFetched(const QString &sourceName, const QList<NewsItem> &articles);
    
    // عند حدوث خطأ في مصدر معين
    void fetchError(const QString &sourceName, const QString &error);
    
    // عند الانتهاء من جلب جميع المصادر (مع قائمة بجميع الأخبار)
    void allFeedsFetched(const QList<NewsItem> &allArticles);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *networkManager;
    DatabaseManager *dbManager;
    
    // قائمة المصادر المعلقة (لحساب عدد المصادر المنتهية)
    QList<QPair<QString, QString>> pendingSources;
    int totalFeeds;
    int finishedFeeds;
    
    // تخزين مؤقت لجميع الأخبار المجلوبة
    QList<NewsItem> allFetchedArticles;

    // دوال مساعدة
    QList<NewsItem> parseFeed(QXmlStreamReader &xml, const QString &sourceName);
    QString extractImageUrl(const QString &description, const QString &sourceUrl);
    void saveArticles(const QList<NewsItem> &articles);
    QString cleanHtml(const QString &html);
    QString extractTextFromElement(QXmlStreamReader &xml);
};

#endif // RSSFETCHER_H
