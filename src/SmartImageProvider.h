#ifndef SMARTIMAGEPROVIDER_H
#define SMARTIMAGEPROVIDER_H

#include <QObject>
#include <QPixmap>
#include <QString>
#include <QMap>
#include <QStringList>
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
class SmartImageProvider : public QObject
{
    Q_OBJECT
public:
    static SmartImageProvider* instance();
    QPixmap getImageForNews(const NewsItem &item, int width, int height, const QString &langCode);

    // دوال جديدة للذكاء
    static QString resourcesPath();
    void learnFromNews(const NewsItem &item, const QString &correctCategory, const QString &langCode);

private:
    explicit SmartImageProvider(QObject *parent = nullptr);
    static SmartImageProvider *m_instance;

    void loadMappingsFromJson();
    QStringList getTopIcons(const QString &text, int maxCount = 3) const;
    bool tryLoadSvg(const QString &iconName, QPixmap &outPixmap, int width, int height) const;
    bool loadBackgroundSvg(const QString &category, QPixmap &outPixmap, int width, int height) const;
    
    void drawCategoryBackground(QPainter &painter, const QString &category, int width, int height);
    void drawIconForName(const QString &name, QPainter &painter, const QRect &rect);
    
    // أيقونات افتراضية
    void drawOilIcon(QPainter &painter, const QRect &rect);
    void drawFootballIcon(QPainter &painter, const QRect &rect);
    void drawWarIcon(QPainter &painter, const QRect &rect);
    void drawHealthIcon(QPainter &painter, const QRect &rect);
    void drawTechIcon(QPainter &painter, const QRect &rect);
    void drawEconomyIcon(QPainter &painter, const QRect &rect);
    void drawCultureIcon(QPainter &painter, const QRect &rect);
    void drawEnvironmentIcon(QPainter &painter, const QRect &rect);
    void drawPoliticsIcon(QPainter &painter, const QRect &rect);
    void drawDefaultIcon(QPainter &painter, const QRect &rect);
    
    // دائرة ملونة ذكية
    void drawColoredCircle(QPainter &painter, const QRect &rect, const QColor &color, const QString &letter);

    QMap<QString, QStringList> allKeywords;
    QMap<QString, QColor> categoryColors;
    void initCategoryColors();
};

#endif // SMARTIMAGEPROVIDER_H
