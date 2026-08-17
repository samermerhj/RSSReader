#include <QApplication>
#include <QDebug>
#include <QLocale>
#include <QTranslator>
#include <QDir>
#include <QLibraryInfo>
#include <QSettings>
#include <QMessageBox>
#include "MainWindow.h"
#include "ResourceManager.h"  // 🔥 تمت الإضافة

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
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // ---------- 1. إعدادات التطبيق ----------
    QCoreApplication::setApplicationName("RSSReader");
    QCoreApplication::setOrganizationName("MyCompany");
    QCoreApplication::setOrganizationDomain("rssreader.com");

    // ---------- 2. قراءة اللغة المفضلة من الإعدادات ----------
    QSettings settings("MyCompany", "RSSReader");
    QString savedLang = settings.value("language", "").toString();

    // إذا لم تكن محفوظة، نعتمد على لغة النظام
    QString lang = savedLang.isEmpty() ? QLocale::system().name() : savedLang;
    QString langCode = lang.left(2); // أول حرفين (ar, fr, en, zh, ru, es, de...)

    qDebug() << "🌐 اللغة المختارة:" << lang << "(الكود:" << langCode << ")";

    // ---------- 3. تحميل ترجمة التطبيق ----------
    QTranslator appTranslator;
    bool translationLoaded = false;

    // 🔥 الحصول على مسار الترجمات من ResourceManager
    QString transPath = ResourceManager::getTranslationsPath();

    // محاولة تحميل الترجمة بناءً على اللغة
    QString translationFile = transPath + "/RSSReader_" + lang;
    QString translationFileShort = transPath + "/RSSReader_" + langCode;

    // محاولة تحميل الملف الكامل (مثل RSSReader_ar_SA) أولاً، ثم الملف المختصر (RSSReader_ar)
    if (appTranslator.load(translationFile) || appTranslator.load(translationFileShort)) {
        app.installTranslator(&appTranslator);
        translationLoaded = true;
        qDebug() << "✅ تم تحميل الترجمة للغة:" << lang;
    } else {
        // محاولة تحميل اللغة الإنجليزية كحل احتياطي
        QString enFile = transPath + "/RSSReader_en";
        if (appTranslator.load(enFile)) {
            app.installTranslator(&appTranslator);
            translationLoaded = true;
            qDebug() << "✅ تم تحميل الترجمة الإنجليزية (احتياطي).";
        } else {
            qWarning() << "⚠️ فشل تحميل أي ترجمة من:" << transPath;
        }
    }

    // ---------- 4. تحميل ترجمة Qt (أزرار الحوارات) ----------
    QTranslator qtTranslator;
    QString qtTranslationsPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);

    // محاولة تحميل ترجمة Qt للغة المحددة
    QString qtTranslationFile = QString("qt_%1").arg(lang);
    QString qtTranslationFileShort = QString("qt_%1").arg(langCode);

    // نبحث في مسار النظام أولاً، ثم في مجلد الترجمات الخاص بنا
    if (!qtTranslationsPath.isEmpty()) {
        if (qtTranslator.load(qtTranslationFile, qtTranslationsPath) ||
            qtTranslator.load(qtTranslationFileShort, qtTranslationsPath) ||
            qtTranslator.load(qtTranslationFile, transPath) ||
            qtTranslator.load(qtTranslationFileShort, transPath)) {
            app.installTranslator(&qtTranslator);
            qDebug() << "✅ تم تحميل ترجمة Qt للغة:" << lang;
        } else {
            qDebug() << "ℹ️ لم يتم العثور على ترجمة Qt للغة:" << lang;
        }
    }

    // ---------- 5. اتجاه الواجهة ----------
    // اللغات التي تكتب من اليمين لليسار
    if (langCode == "ar" || langCode == "fa" || langCode == "he" || langCode == "ur") {
        app.setLayoutDirection(Qt::RightToLeft);
        qDebug() << "🔄 اتجاه الواجهة: من اليمين إلى اليسار (RTL)";
    } else {
        app.setLayoutDirection(Qt::LeftToRight);
        qDebug() << "🔄 اتجاه الواجهة: من اليسار إلى اليمين (LTR)";
    }

    // ---------- 6. إنشاء وعرض النافذة الرئيسية ----------
    qDebug() << "🚀 بدء تشغيل تطبيق RSS Reader...";

    MainWindow w;
    w.show();

    qDebug() << "✅ ظهرت النافذة الرئيسية";

    return app.exec();
}
