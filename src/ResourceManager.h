#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <QString>

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

class ResourceManager
{
public:
    static QString getBasePath();               // ~/rsss/
    static QString getResourcesPath();          // ~/rsss/resources/
    static QString getWritableResourcesPath();  // ~/rsss/resources/ (نفسه)
    static QString getTranslationsPath();       // ~/rsss/translations/
    static QString getCachePath();              // ~/rsss/cache/
    static QString getDataPath();               // ~/rsss/ (لقاعدة البيانات)
};

#endif // RESOURCEMANAGER_H
