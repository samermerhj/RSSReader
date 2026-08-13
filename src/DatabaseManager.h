#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QList>
#include <QString>
#include <QVariant>

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
struct NewsItem {
    int id = -1;
    QString source;         // اسم المصدر أو قائمة المصادر مفصولة بفاصلة
    QString title;
    QString description;
    QString link;
    QString pubDate;
    QDate archiveDate;
    QStringList getDistinctDates() const;
    QString imageUrl;       // رابط الصورة الرئيسية (اختياري)
};

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseManager(const QString &dbPath = "rss_archive.db", QObject *parent = nullptr);
    ~DatabaseManager();

    // ------ إدارة المصادر ------
    QList<QPair<QString, QString>> getSources() const;   // (name, url)
    bool addSource(const QString &name, const QString &url);
    bool deleteSource(const QString &url);

    // ------ حفظ واسترجاع الأخبار ------
    bool saveArticle(const NewsItem &item);              // يحفظ الخبر (يتجنب التكرار حسب الرابط)
    QList<NewsItem> getNewsByDate(const QDate &date) const;
    QList<NewsItem> getLatestNews(int limit = 100) const;
    QStringList getDistinctDates() const;
    
    // ------ (اختياري) حذف الأخبار القديمة ------ 
    bool deleteOldNews(int daysToKeep = 30);

private:
    QSqlDatabase db;
    void initDB();
    bool tableExists(const QString &tableName) const;
    bool columnExists(const QString &tableName, const QString &columnName) const;
    void addColumnIfMissing(const QString &tableName, const QString &columnName, const QString &type);
};

#endif // DATABASEMANAGER_H
