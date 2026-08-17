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
    // المسار الأساسي
    static QString getBasePath();

    // الموارد الثابتة (للقراءة فقط)
    static QString getResourcesPath();

    // الموارد القابلة للكتابة (مثل icon_mappings.json)
    static QString getWritableResourcesPath();

    // الترجمات
    static QString getTranslationsPath();

    // الكاش (الصور المؤقتة)
    static QString getCachePath();

    // البيانات الدائمة (قاعدة البيانات)
    static QString getDataPath();

    // 🔥 تهيئة OpenSSL (خاص بويندوز، لا تؤثر على لينكس)
    static void initOpenSSL();
};

#endif // RESOURCEMANAGER_H
