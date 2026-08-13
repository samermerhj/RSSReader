#include "DatabaseManager.h"
#include "ResourceManager.h"
#include <QDebug>
#include <QDir>
#include <QDateTime>
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
DatabaseManager::DatabaseManager(const QString &dbPath, QObject *parent)
    : QObject(parent)
{
    // تجاهل dbPath الوارد واستخدم المسار الموحد
    QString effectivePath = ResourceManager::getDataPath() + "/rss_archive.db";
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(effectivePath);
    if (!db.open()) {
        qWarning() << "فشل فتح قاعدة البيانات:" << db.lastError().text();
        return;
    }
    initDB();
}

DatabaseManager::~DatabaseManager()
{
    if (db.isOpen()) {
        db.close();
    }
}

void DatabaseManager::initDB()
{
    // إنشاء جدول الأخبار (مع عمود الصورة)
    if (!tableExists("news_archive")) {
        QSqlQuery query(db);
        QString createSQL = R"(
            CREATE TABLE news_archive (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                source TEXT,
                title TEXT,
                description TEXT,
                link TEXT UNIQUE,
                pub_date TEXT,
                archive_date TEXT,
                image_url TEXT
            )
        )";
        if (!query.exec(createSQL)) {
            qWarning() << "فشل إنشاء جدول news_archive:" << query.lastError().text();
        }
    } else {
        // التأكد من وجود عمود image_url (للتحديث من إصدار سابق)
        if (!columnExists("news_archive", "image_url")) {
            addColumnIfMissing("news_archive", "image_url", "TEXT");
        }
    }

    // إنشاء جدول المصادر
    if (!tableExists("rss_sources")) {
        QSqlQuery query(db);
        QString createSources = R"(
            CREATE TABLE rss_sources (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT,
                url TEXT UNIQUE
            )
        )";
        if (!query.exec(createSources)) {
            qWarning() << "فشل إنشاء جدول rss_sources:" << query.lastError().text();
        }

        // إدراج مصادر افتراضية (يمكن للمستخدم تعديلها لاحقاً)
        QList<QPair<QString, QString>> defaults = {
        {"BBC علوم", "http://www.bbc.co.uk/arabic/scienceandtech/index.xml"},
        {"روسيا اليوم", "https://arabic.rt.com/rss/"},
        {"سكاي نيوز عربية", "https://www.skynewsarabia.com/rss"}
    };
        for (const auto &pair : defaults) {
            addSource(pair.first, pair.second);
        }
    }
}

bool DatabaseManager::tableExists(const QString &tableName) const
{
    QSqlQuery query(db);
    query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=:name");
    query.bindValue(":name", tableName);
    return query.exec() && query.next();
}

bool DatabaseManager::columnExists(const QString &tableName, const QString &columnName) const
{
    QSqlQuery query(db);
    query.prepare(QString("PRAGMA table_info(%1)").arg(tableName));
    if (!query.exec()) return false;
    while (query.next()) {
        if (query.value(1).toString() == columnName) {
            return true;
        }
    }
    return false;
}

void DatabaseManager::addColumnIfMissing(const QString &tableName, const QString &columnName, const QString &type)
{
    if (!columnExists(tableName, columnName)) {
        QSqlQuery query(db);
        QString sql = QString("ALTER TABLE %1 ADD COLUMN %2 %3").arg(tableName).arg(columnName).arg(type);
        if (!query.exec(sql)) {
            qWarning() << "فشل إضافة العمود" << columnName << ":" << query.lastError().text();
        }
    }
}

// ------ إدارة المصادر ------

QList<QPair<QString, QString>> DatabaseManager::getSources() const
{
    QList<QPair<QString, QString>> sources;
    QSqlQuery query(db);
    if (query.exec("SELECT name, url FROM rss_sources ORDER BY name")) {
        while (query.next()) {
            sources.append({query.value(0).toString(), query.value(1).toString()});
        }
    } else {
        qWarning() << "getSources فشل:" << query.lastError().text();
    }
    return sources;
}

bool DatabaseManager::addSource(const QString &name, const QString &url)
{
    QSqlQuery query(db);
    query.prepare("INSERT OR IGNORE INTO rss_sources (name, url) VALUES (:name, :url)");
    query.bindValue(":name", name);
    query.bindValue(":url", url);
    if (!query.exec()) {
        qWarning() << "addSource فشل:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::deleteSource(const QString &url)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM rss_sources WHERE url = :url");
    query.bindValue(":url", url);
    if (!query.exec()) {
        qWarning() << "deleteSource فشل:" << query.lastError().text();
        return false;
    }
    return true;
}

// ------ حفظ واسترجاع الأخبار ------

bool DatabaseManager::saveArticle(const NewsItem &item)
{
    // التحقق من التكرار باستخدام الرابط (link)
    QSqlQuery check(db);
    check.prepare("SELECT id FROM news_archive WHERE link = :link");
    check.bindValue(":link", item.link);
    if (check.exec() && check.next()) {
        return true; // الخبر موجود مسبقاً، لا نقوم بتحديثه
    }

    QSqlQuery query(db);
    query.prepare(R"(
        INSERT INTO news_archive (source, title, description, link, pub_date, archive_date, image_url)
        VALUES (:source, :title, :desc, :link, :pub, :arch, :img)
    )");
    query.bindValue(":source", item.source);
    query.bindValue(":title", item.title);
    query.bindValue(":desc", item.description);
    query.bindValue(":link", item.link);
    query.bindValue(":pub", item.pubDate);
    query.bindValue(":arch", QDate::currentDate().toString(Qt::ISODate));
    query.bindValue(":img", item.imageUrl);
    if (!query.exec()) {
        qWarning() << "saveArticle فشل:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<NewsItem> DatabaseManager::getNewsByDate(const QDate &date) const
{
    QList<NewsItem> items;
    QSqlQuery query(db);
    query.prepare(R"(
        SELECT id, source, title, description, link, pub_date, archive_date, image_url
        FROM news_archive
        WHERE archive_date = :date
        ORDER BY pub_date DESC
    )");
    query.bindValue(":date", date.toString(Qt::ISODate));
    if (!query.exec()) {
        qWarning() << "getNewsByDate فشل:" << query.lastError().text();
        return items;
    }
    while (query.next()) {
        NewsItem item;
        item.id = query.value(0).toInt();
        item.source = query.value(1).toString();
        item.title = query.value(2).toString();
        item.description = query.value(3).toString();
        item.link = query.value(4).toString();
        item.pubDate = query.value(5).toString();
        item.archiveDate = QDate::fromString(query.value(6).toString(), Qt::ISODate);
        item.imageUrl = query.value(7).toString();
        items.append(item);
    }
    return items;
}

QList<NewsItem> DatabaseManager::getLatestNews(int limit) const
{
    QList<NewsItem> items;
    QSqlQuery query(db);
    query.prepare(R"(
        SELECT id, source, title, description, link, pub_date, archive_date, image_url
        FROM news_archive
        ORDER BY archive_date DESC, pub_date DESC
        LIMIT :limit
    )");
    query.bindValue(":limit", limit);
    if (!query.exec()) {
        qWarning() << "getLatestNews فشل:" << query.lastError().text();
        return items;
    }
    while (query.next()) {
        NewsItem item;
        item.id = query.value(0).toInt();
        item.source = query.value(1).toString();
        item.title = query.value(2).toString();
        item.description = query.value(3).toString();
        item.link = query.value(4).toString();
        item.pubDate = query.value(5).toString();
        item.archiveDate = QDate::fromString(query.value(6).toString(), Qt::ISODate);
        item.imageUrl = query.value(7).toString();
        items.append(item);
    }
    return items;
}

bool DatabaseManager::deleteOldNews(int daysToKeep)
{
    QDate cutoff = QDate::currentDate().addDays(-daysToKeep);
    QSqlQuery query(db);
    query.prepare("DELETE FROM news_archive WHERE archive_date < :cutoff");
    query.bindValue(":cutoff", cutoff.toString(Qt::ISODate));
    if (!query.exec()) {
        qWarning() << "deleteOldNews فشل:" << query.lastError().text();
        return false;
    }
    return true;
}

QStringList DatabaseManager::getDistinctDates() const
{
    QStringList dates;
    QSqlQuery query(db);
    if (query.exec("SELECT DISTINCT archive_date FROM news_archive ORDER BY archive_date DESC")) {
        while (query.next()) {
            dates.append(query.value(0).toString());
        }
    } else {
        qWarning() << "getDistinctDates فشل:" << query.lastError().text();
    }
    return dates;
}
