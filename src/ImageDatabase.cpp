#include "ImageDatabase.h"
#include <QPainter>
#include <QDebug>
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
ImageDatabase* ImageDatabase::m_instance = nullptr;

ImageDatabase* ImageDatabase::instance()
{
    if (!m_instance) {
        m_instance = new ImageDatabase();
    }
    return m_instance;
}

ImageDatabase::ImageDatabase(QObject *parent)
    : QObject(parent)
{
    loadImages();
}

void ImageDatabase::loadImages()
{
    categoryColors = {
        {"سياسي", "#e74c3c"},
        {"رياضي", "#2ecc71"},
        {"صحي", "#1abc9c"},
        {"اقتصادي", "#f39c12"},
        {"تكنولوجيا", "#3498db"},
        {"عسكري", "#2c3e50"},
        {"بيئي", "#27ae60"},
        {"ثقافي", "#9b59b6"},
        {"default", "#95a5a6"}
    };
}

QPixmap ImageDatabase::createCategoryImage(const QString &category, int width, int height)
{
    QString colorName = categoryColors.value(category, "#95a5a6");
    QColor color = QColor(colorName);
    
    QPixmap pixmap(width, height);
    pixmap.fill(color.lighter(160));
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // رسم مستطيل مع زوايا دائرية
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(10, 10, width - 20, height - 20, 15, 15);
    
    // رسم رمز
    painter.setPen(QPen(Qt::white, 3));
    painter.setFont(QFont("Segoe UI", width / 4, QFont::Bold));
    QMap<QString, QString> symbols = {
        {"سياسي", "🏛"},
        {"رياضي", "⚽"},
        {"صحي", "🏥"},
        {"اقتصادي", "💰"},
        {"تكنولوجيا", "💻"},
        {"عسكري", "⚔"},
        {"بيئي", "🌿"},
        {"ثقافي", "🎭"},
        {"default", "📰"}
    };
    QString symbol = symbols.value(category, "📰");
    painter.drawText(QRect(0, 0, width, height), Qt::AlignCenter, symbol);
    
    return pixmap;
}

QPixmap ImageDatabase::getImageForCategory(const QString &category, int width, int height)
{
    if (imageCache.contains(category)) {
        return imageCache[category].scaled(width, height, 
                                           Qt::KeepAspectRatioByExpanding, 
                                           Qt::SmoothTransformation);
    }
    
    QPixmap pixmap = createCategoryImage(category, width, height);
    imageCache[category] = pixmap;
    return pixmap;
}

QPixmap ImageDatabase::getDefaultImage(int width, int height)
{
    return getImageForCategory("default", width, height);
}

QStringList ImageDatabase::getCategories() const
{
    return categoryColors.keys();
}
