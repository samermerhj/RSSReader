#include "ResourceManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

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
 
QString ResourceManager::getResourcesPath()
{
    // 1. أثناء التطوير (من مجلد البناء)
    QString devPath = QCoreApplication::applicationDirPath() + "/resources";
    if (QDir(devPath).exists())
        return devPath;

    // 2. أثناء التثبيت (عبر .deb)
    QString installPath = "/usr/share/RSSReader/resources";
    if (QDir(installPath).exists())
        return installPath;

    // 3. مسار احتياطي
    return devPath; // أو يمكنك إرجاع QString()
}

QString ResourceManager::getTranslationsPath()
{
    QString devPath = QCoreApplication::applicationDirPath() + "/translations";
    if (QDir(devPath).exists())
        return devPath;

    QString installPath = "/usr/share/RSSReader/translations";
    if (QDir(installPath).exists())
        return installPath;

    return devPath;
}

QString ResourceManager::getCachePath()
{
    // استخدام المسار المخصص للتطبيق في مجلد المستخدم
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheDir.isEmpty())
        cacheDir = QCoreApplication::applicationDirPath() + "/cache";
    return cacheDir;
}
