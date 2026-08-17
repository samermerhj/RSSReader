#include "ResourceManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QFile>

/**
 * @brief ResourceManager_win.cpp
 * تنفيذ خاص بنظام ويندوز فقط.
 * يستخدم مسارات ويندوز القياسية (AppData) بدلاً من ~/rsss/.
 */

// المسار الأساسي للتطبيق (مجلد قابل للكتابة)
QString ResourceManager::getBasePath()
{
#ifdef Q_OS_WIN
    // استخدام LocalAppData (مجلد خاص بالتطبيق في مساحة المستخدم)
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (path.isEmpty()) {
        // احتياطي: إذا لم يعمل QStandardPaths
        path = QDir::homePath() + "/AppData/Local/RSSReader";
    }
    return path;
#else
    // هذا الملف لن يُستخدم إلا على ويندوز، لكن نضعها للسلامة
    return QDir::homePath() + "/rsss";
#endif
}

// الموارد الثابتة (للقراءة فقط) - تبحث بجانب الملف التنفيذي أولاً
QString ResourceManager::getResourcesPath()
{
    // 1. نبحث في مجلد الملف التنفيذي (للتطوير أو التثبيت المحمول)
    QString appDir = QCoreApplication::applicationDirPath();
    QString path = appDir + "/resources";
    if (QDir(path).exists()) {
        return path;
    }

    // 2. إذا لم نجد، نستخدم المسار في AppData (احتياطي)
    return getBasePath() + "/resources";
}

// الموارد القابلة للكتابة (مثل icon_mappings.json) - في AppData
QString ResourceManager::getWritableResourcesPath()
{
    return getBasePath() + "/resources";
}

// الترجمات - تبحث بجانب الملف التنفيذي أولاً
QString ResourceManager::getTranslationsPath()
{
    // 1. نبحث في مجلد الملف التنفيذي
    QString appDir = QCoreApplication::applicationDirPath();
    QString path = appDir + "/translations";
    if (QDir(path).exists()) {
        return path;
    }

    // 2. إذا لم نجد، نستخدم المسار في AppData (احتياطي)
    return getBasePath() + "/translations";
}

// الكاش (الصور المؤقتة) - في AppData
QString ResourceManager::getCachePath()
{
    return getBasePath() + "/cache";
}

// البيانات الدائمة (قاعدة البيانات) - في AppData
QString ResourceManager::getDataPath()
{
    return getBasePath();
}

// 🔥 تهيئة OpenSSL (خاص بويندوز)
void ResourceManager::initOpenSSL()
{
#ifdef Q_OS_WIN
    qDebug() << "🔐 تهيئة OpenSSL لويندوز...";

    // 1. إجبار استخدام TLS 1.2 أو أحدث
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::TlsV1_2OrLater);
    QSslConfiguration::setDefaultConfiguration(config);

    // 2. تحديد مسار OpenSSL (نسبة إلى مجلد التطبيق)
    QString appDir = QCoreApplication::applicationDirPath();
    QString opensslConf = appDir + "/openssl.cnf";
    if (QFile::exists(opensslConf)) {
        qputenv("OPENSSL_CONF", opensslConf.toUtf8());
        qDebug() << "✅ تم تعيين OPENSSL_CONF إلى:" << opensslConf;
    }

    // 3. التحقق من دعم OpenSSL
    if (QSslSocket::supportsSsl()) {
        qDebug() << "✅ OpenSSL مدعوم (الإصدار:" << QSslSocket::sslLibraryVersionString() << ")";
    } else {
        qWarning() << "⚠️ OpenSSL غير مدعوم! تأكد من وجود libcrypto-1_1-x64.dll و libssl-1_1-x64.dll بجانب الملف التنفيذي.";
    }

    // 4. عرض إصدار OpenSSL المطلوب للبناء (للتأكد من التوافق)
    qDebug() << "📦 إصدار OpenSSL المطلوب للبناء:" << QSslSocket::sslLibraryBuildVersionString();
#endif
}
