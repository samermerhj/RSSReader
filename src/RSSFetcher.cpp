#include "RSSFetcher.h"
#include <QUrl>
#include <QDateTime>
#include <QRegularExpression>
#include <QDebug>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QSslSocket>

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

RSSFetcher::RSSFetcher(DatabaseManager *dbManager, QObject *parent)
    : QObject(parent)
    , dbManager(dbManager)
    , totalFeeds(0)
    , finishedFeeds(0)
{
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &RSSFetcher::onReplyFinished);
}

void RSSFetcher::fetchFeed(const QString &url, const QString &sourceName)
{
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

    // ---------- إصلاحات HTTPS ----------
    // 1. تفعيل اتباع إعادة التوجيه (Redirects)
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    // 2. فرض استخدام TLS 1.2 أو أحدث (مهم جداً لويندوز 7)
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    request.setSslConfiguration(sslConfig);
    // -----------------------------------

    QNetworkReply *reply = networkManager->get(request);
    reply->setProperty("sourceName", sourceName);
    reply->setProperty("sourceUrl", url);
}

void RSSFetcher::fetchAllFeeds(const QList<QPair<QString, QString>> &sources)
{
    if (sources.isEmpty()) {
        emit allFeedsFetched(QList<NewsItem>());
        return;
    }

    pendingSources = sources;
    totalFeeds = sources.size();
    finishedFeeds = 0;
    allFetchedArticles.clear();

    networkManager->setTransferTimeout(10000);  // 10 seconds

    for (const auto &pair : sources) {
        fetchFeed(pair.second, pair.first);
    }
}

void RSSFetcher::onReplyFinished(QNetworkReply *reply)
{
    QString sourceName = reply->property("sourceName").toString();
    QString sourceUrl = reply->property("sourceUrl").toString();
    
    if (sourceName.isEmpty()) {
        reply->deleteLater();
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        emit fetchError(sourceName, reply->errorString());
    } else {
        QByteArray data = reply->readAll();
        QXmlStreamReader xml(data);
        QList<NewsItem> articles = parseFeed(xml, sourceName);
        
        for (NewsItem &item : articles) {
            if (item.imageUrl.isEmpty() && !item.description.isEmpty()) {
                item.imageUrl = extractImageUrl(item.description, sourceUrl);
            }
        }
        
        if (!articles.isEmpty()) {
            saveArticles(articles);
            allFetchedArticles.append(articles);
            emit feedFetched(sourceName, articles);
        } else {
            emit fetchError(sourceName, tr("لا توجد مقالات في هذا المصدر"));
        }
    }

    reply->deleteLater();
    finishedFeeds++;
    
    if (finishedFeeds == totalFeeds) {
        emit allFeedsFetched(allFetchedArticles);
    }
}

QList<NewsItem> RSSFetcher::parseFeed(QXmlStreamReader &xml, const QString &sourceName)
{
    QList<NewsItem> items;
    
    qDebug() << "بدء تحليل الخلاصة:" << sourceName;
    
    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        
        if (token == QXmlStreamReader::StartDocument) {
            continue;
        }
        
        if (token == QXmlStreamReader::StartElement) {
            QString elementName = xml.name().toString();
            qDebug() << "العنصر الجذر:" << elementName;
            
            // ====== RSS 2.0 ======
            if (elementName == "rss") {
                while (!xml.atEnd() && !xml.hasError()) {
                    xml.readNext();
                    if (xml.isStartElement() && xml.name() == "channel") {
                        break;
                    }
                }
                
                while (!xml.atEnd() && !xml.hasError()) {
                    xml.readNext();
                    if (xml.isStartElement() && xml.name() == "item") {
                        NewsItem item;
                        item.source = sourceName;
                        
                        while (!xml.atEnd() && !xml.hasError()) {
                            xml.readNext();
                            
                            if (xml.isStartElement()) {
                                QString tag = xml.name().toString();
                                
                                if (tag == "title") {
                                    item.title = xml.readElementText().trimmed();
                                } else if (tag == "description") {
                                    item.description = xml.readElementText().trimmed();
                                } else if (tag == "link") {
                                    item.link = xml.readElementText().trimmed();
                                } else if (tag == "pubDate") {
                                    item.pubDate = xml.readElementText().trimmed();
                                } else if (tag == "enclosure") {
                                    QString type = xml.attributes().value("type").toString();
                                    if (type.startsWith("image/")) {
                                        item.imageUrl = xml.attributes().value("url").toString();
                                    }
                                    xml.skipCurrentElement();
                                } else if (tag == "media:content" || tag == "content") {
                                    QString medium = xml.attributes().value("medium").toString();
                                    if (medium == "image" || medium.isEmpty()) {
                                        item.imageUrl = xml.attributes().value("url").toString();
                                    }
                                    xml.skipCurrentElement();
                                } else if (tag == "media:thumbnail") {
                                    if (item.imageUrl.isEmpty()) {
                                        item.imageUrl = xml.attributes().value("url").toString();
                                    }
                                    xml.skipCurrentElement();
                                }
                            } else if (xml.isEndElement() && xml.name() == "item") {
                                break;
                            }
                        }
                        
                        if (!item.title.isEmpty() && !item.link.isEmpty()) {
                            items.append(item);
                            qDebug() << "تم العثور على خبر:" << item.title.left(50) << "...";
                        }
                    } else if (xml.isEndElement() && xml.name() == "channel") {
                        break;
                    }
                }
            }
            // ====== Atom ======
            else if (elementName == "feed") {
                while (!xml.atEnd() && !xml.hasError()) {
                    xml.readNext();
                    
                    if (xml.isStartElement() && xml.name() == "entry") {
                        NewsItem item;
                        item.source = sourceName;
                        
                        while (!xml.atEnd() && !xml.hasError()) {
                            xml.readNext();
                            
                            if (xml.isStartElement()) {
                                QString tag = xml.name().toString();
                                
                                if (tag == "title") {
                                    item.title = xml.readElementText().trimmed();
                                } else if (tag == "summary" || tag == "content") {
                                    item.description = xml.readElementText().trimmed();
                                } else if (tag == "link") {
                                    QString rel = xml.attributes().value("rel").toString();
                                    QString href = xml.attributes().value("href").toString();
                                    if (rel == "alternate" && item.link.isEmpty()) {
                                        item.link = href;
                                    } else if (rel == "enclosure" && item.imageUrl.isEmpty()) {
                                        QString type = xml.attributes().value("type").toString();
                                        if (type.startsWith("image/")) {
                                            item.imageUrl = href;
                                        }
                                    } else if (item.link.isEmpty() && !href.isEmpty()) {
                                        item.link = href;
                                    }
                                    xml.skipCurrentElement();
                                } else if (tag == "published" || tag == "updated") {
                                    item.pubDate = xml.readElementText().trimmed();
                                }
                            } else if (xml.isEndElement() && xml.name() == "entry") {
                                break;
                            }
                        }
                        
                        if (!item.title.isEmpty() && !item.link.isEmpty()) {
                            items.append(item);
                            qDebug() << "تم العثور على خبر Atom:" << item.title.left(50) << "...";
                        }
                    } else if (xml.isEndElement() && xml.name() == "feed") {
                        break;
                    }
                }
            } else {
                qDebug() << "تنسيق غير معروف:" << elementName;
                xml.skipCurrentElement();
            }
        }
    }
    
    if (xml.hasError()) {
        qDebug() << "خطأ في تحليل XML:" << xml.errorString();
    }
    
    qDebug() << "انتهى تحليل الخلاصة، تم العثور على" << items.size() << "خبراً";
    return items;
}

QString RSSFetcher::extractImageUrl(const QString &description, const QString &sourceUrl)
{
    if (description.isEmpty()) return QString();
    
    QRegularExpression regex("<img[^>]+src\\s*=\\s*[\"']([^\"']+)[\"']", 
                            QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = regex.globalMatch(description);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString url = match.captured(1);
        
        if (!url.isEmpty() && !url.startsWith("data:")) {
            if (!url.startsWith("http://") && !url.startsWith("https://")) {
                QUrl baseUrl(sourceUrl);
                QString basePath = baseUrl.toString(QUrl::RemovePath);
                if (!basePath.endsWith('/')) basePath += '/';
                
                if (url.startsWith("//")) {
                    url = "https:" + url;
                } else if (url.startsWith("/")) {
                    url = baseUrl.scheme() + "://" + baseUrl.host() + url;
                } else {
                    url = basePath + url;
                }
            }
            return url;
        }
    }
    
    return QString();
}

QString RSSFetcher::cleanHtml(const QString &html)
{
    QString clean = html;
    clean.remove(QRegularExpression("<[^>]*>"));
    clean.replace("&nbsp;", " ");
    clean.replace("&amp;", "&");
    clean.replace("&lt;", "<");
    clean.replace("&gt;", ">");
    clean.replace("&quot;", "\"");
    clean.replace("&apos;", "'");
    clean.replace(QRegularExpression("\\s+"), " ");
    return clean.trimmed();
}

QString RSSFetcher::extractTextFromElement(QXmlStreamReader &xml)
{
    QString text;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isEndElement()) break;
        if (xml.isCharacters()) {
            text += xml.text().toString();
        }
    }
    return text.trimmed();
}

void RSSFetcher::saveArticles(const QList<NewsItem> &articles)
{
    for (const NewsItem &item : articles) {
        NewsItem cleanItem = item;
        cleanItem.description = cleanHtml(item.description);
        dbManager->saveArticle(cleanItem);
    }
}
