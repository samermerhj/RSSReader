#ifndef IMAGEDATABASE_H
#define IMAGEDATABASE_H

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QString>
#include <QPixmap>
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
class ImageDatabase : public QObject
{
    Q_OBJECT
public:
    static ImageDatabase* instance();
    
    void loadImages();
    QPixmap getImageForCategory(const QString &category, int width = 400, int height = 250);
    QPixmap getDefaultImage(int width = 400, int height = 250);
    QStringList getCategories() const;

private:
    explicit ImageDatabase(QObject *parent = nullptr);
    static ImageDatabase *m_instance;
    
    QMap<QString, QPixmap> imageCache;
    QMap<QString, QString> categoryColors;
    
    QPixmap createCategoryImage(const QString &category, int width, int height);
};

#endif // IMAGEDATABASE_H
