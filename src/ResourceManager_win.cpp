#include "ResourceManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QFileInfo>

/**
 * @brief ResourceManager_win.cpp
 * تنفيذ خاص بنظام ويندوز.
 * يعمل تماماً مثل ResourceManager.cpp (يستخدم ~/rsss/) مع إضافة آلية لنقل الملفات.
 */

// المسار الأساسي (~/rsss/) مع إنشاء تلقائي
QString ResourceManager::getBasePath()
{
    QString path = QDir::homePath() + "/rsss";
    QDir dir(path);
    if (!dir.exists()) {
        if (dir.mkpath(".")) {
            qDebug() << "✅ تم إنشاء مجلد rsss في:" << path;
        } else {
            qWarning() << "❌ تعذر إنشاء مجلد rsss في:" << path;
        }
    }
    return path;
}

// الموارد - تعيد ~/rsss/resources/، وتنقل الملفات من مجلد exe إذا لزم الأمر
QString ResourceManager::getResourcesPath()
{
    QString base = getBasePath();
    QString resourcesPath = base + "/resources";

    // إذا لم تكن الموارد موجودة، حاول نقلها من مجلد exe
    QDir resDir(resourcesPath);
    if (!resDir.exists()) {
        QString sourcePath = QCoreApplication::applicationDirPath() + "/resources";
        if (QDir(sourcePath).exists()) {
            qDebug() << "📂 نقل resources من:" << sourcePath << "إلى:" << resourcesPath;
            // محاولة نقل المجلد كاملاً
            if (QDir().rename(sourcePath, resourcesPath)) {
                qDebug() << "✅ تم نقل resources بنجاح";
            } else {
                // فشل النقل، نقوم بنسخ الملفات
                qWarning() << "❌ فشل نقل resources، سيتم نسخ الملفات";
                QDir sourceDir(sourcePath);
                for (const QString &file : sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
                    QString src = sourcePath + "/" + file;
                    QString dst = resourcesPath + "/" + file;
                    if (QFileInfo(src).isFile()) {
                        QFile::copy(src, dst);
                    } else {
                        QDir().mkpath(dst);
                        // نسخ محتويات المجلد الفرعي
                        QDir subSrc(src);
                        for (const QString &subFile : subSrc.entryList(QDir::Files)) {
                            QFile::copy(src + "/" + subFile, dst + "/" + subFile);
                        }
                    }
                }
            }
        }
    }

    return resourcesPath;
}

// الموارد القابلة للكتابة (مثل icon_mappings.json) - في ~/rsss/resources/
QString ResourceManager::getWritableResourcesPath()
{
    return getResourcesPath();
}

// الترجمات - تعيد ~/rsss/translations/، وتنقل الملفات من مجلد exe إذا لزم الأمر
QString ResourceManager::getTranslationsPath()
{
    QString base = getBasePath();
    QString translationsPath = base + "/translations";

    // إذا لم تكن الترجمات موجودة، حاول نقلها من مجلد exe
    QDir transDir(translationsPath);
    if (!transDir.exists()) {
        QString sourcePath = QCoreApplication::applicationDirPath() + "/translations";
        if (QDir(sourcePath).exists()) {
            qDebug() << "📂 نقل translations من:" << sourcePath << "إلى:" << translationsPath;
            if (QDir().rename(sourcePath, translationsPath)) {
                qDebug() << "✅ تم نقل translations بنجاح";
            } else {
                qWarning() << "❌ فشل نقل translations، سيتم نسخ الملفات";
                QDir sourceDir(sourcePath);
                for (const QString &file : sourceDir.entryList(QDir::Files)) {
                    QString src = sourcePath + "/" + file;
                    QString dst = translationsPath + "/" + file;
                    QFile::copy(src, dst);
                }
            }
        }
    }

    return translationsPath;
}

// الكاش (الصور المؤقتة) - في ~/rsss/cache/
QString ResourceManager::getCachePath()
{
    return getBasePath() + "/cache";
}

// البيانات الدائمة (قاعدة البيانات) - في ~/rsss/
QString ResourceManager::getDataPath()
{
    return getBasePath();
}
