#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QMessageBox>
#include <QInputDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QDateTime>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QLinearGradient>
#include <QPalette>
#include <QPropertyAnimation>
#include <QDate>
#include <QRegularExpression>
#include <QCloseEvent>
#include <QTimer>
#include <QApplication>
#include <QShowEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QListWidget>
#include <QDialog>
#include <QSvgRenderer>
#include <QPainter>
#include <QTextEdit>
#include <QClipboard>
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

// دالة مساعدة: تكتشف لغة النص (توضع بعد التضمينات مباشرة)
static QString detectLanguage(const QString &text)
{
    for (const QChar &ch : text) {
        if (ch.unicode() >= 0x0600 && ch.unicode() <= 0x06FF)
            return "ar";
    }
    return "en";
}
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , currentSourceIndex(0)
    , refreshTimer(nullptr)
    , sourceCycleTimer(nullptr)
    , trayIcon(nullptr)
    , trayMenu(nullptr)
    , magazineCheckTimer(nullptr)
    , autoSaveEnabled(false)
    , autoSaveCount(20)
    , deduplicateOnSave(false)
    , maxMagazineArticles(30)
    , retentionDays(30)
    , dailySaveLimit(100)
    , todaySavedCount(0)
    , lastSaveDate(QDate::currentDate())
{
    setWindowTitle(tr("RSS Reader - Advanced Electronic Newspaper"));

    dbManager = new DatabaseManager("rss_archive.db", this);
    fetcher = new RSSFetcher(dbManager, this);
    connect(fetcher, &RSSFetcher::feedFetched, this, &MainWindow::onFeedFetched);
    connect(fetcher, &RSSFetcher::fetchError, this, &MainWindow::onFetchError);
    connect(fetcher, &RSSFetcher::allFeedsFetched, this, &MainWindow::onAllFeedsFetched);

    IconManager::instance();
    ImageDatabase::instance();

    initUI();

    sources = dbManager->getSources();
    updateSourceCombo();

    // ---------- أيقونة التطبيق وعلبة النظام ----------
    QIcon appIcon(":/icons/tray_icon.svg");  // تأكد من وجود الملف icons/tray_icon.png
    setWindowIcon(appIcon);
    QApplication::setWindowIcon(appIcon);
    trayIcon = new QSystemTrayIcon(appIcon, this);

    // ضبط حجم النافذة حسب الشاشة
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect available = screen->availableGeometry();
        int w = static_cast<int>(available.width() * 0.9);
        int h = static_cast<int>(available.height() * 0.85);
        int x = available.x() + (available.width() - w) / 2;
        int y = available.y() + (available.height() - h) / 2;
        setGeometry(x, y, w, h);
    }

    // إعداد قائمة علبة النظام
    trayMenu = new QMenu(this);
    QAction *showAction = trayMenu->addAction(tr("Open Window"));
    connect(showAction, &QAction::triggered, this, [this]() {
        showNormal();
        activateWindow();
    });
    QAction *magazineAction = trayMenu->addAction(tr("Show Magazine"));
    connect(magazineAction, &QAction::triggered, this, &MainWindow::switchToMagazineMode);
    trayMenu->addSeparator();
    QAction *quitAction = trayMenu->addAction(tr("Exit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();
    connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            showNormal();
            activateWindow();
        }
    });

    if (!sources.isEmpty()) {
        currentSourceIndex = 0;
    } else {
        showMessage(tr("No RSS sources found. Please add a source."), "#e74c3c", true);
    }

    // إنشاء المؤقتات
    refreshTimer = new QTimer(this);
    sourceCycleTimer = new QTimer(this);
    magazineCheckTimer = new QTimer(this);

    // تحميل الإعدادات
    QSettings settings("MyCompany", "RSSReader");

    autoSaveEnabled = settings.value("autoSaveEnabled", false).toBool();
    autoSaveCount = settings.value("autoSaveCount", 20).toInt();
    retentionDays = settings.value("retentionDays", 30).toInt();
    deduplicateOnSave = settings.value("deduplicateOnSave", false).toBool();
    maxMagazineArticles = settings.value("maxMagazineArticles", 30).toInt();

    dailySaveLimit = settings.value("dailySaveLimit", 100).toInt();
    todaySavedCount = settings.value("todaySavedCount", 0).toInt();
    lastSaveDate = QDate::fromString(settings.value("lastSaveDate").toString(), Qt::ISODate);
    if (!lastSaveDate.isValid() || lastSaveDate != QDate::currentDate()) {
        todaySavedCount = 0;
        lastSaveDate = QDate::currentDate();
        settings.setValue("todaySavedCount", 0);
        settings.setValue("lastSaveDate", lastSaveDate.toString(Qt::ISODate));
    }

    QStringList timesStr = settings.value("magazine_times", QStringList()).toStringList();
    for (const QString &t : timesStr) {
        QTime time = QTime::fromString(t, "HH:mm");
        if (time.isValid()) magazineTimes.append(time);
    }

    int fg = settings.value("updateIntervalForeground", 30).toInt();
    int bg = settings.value("updateIntervalBackground", 30).toInt();
    int cycle = settings.value("sourceCycleInterval", 30).toInt();

    refreshTimer->setInterval(fg * 60 * 1000);
    sourceCycleTimer->setInterval(cycle * 1000);
    magazineCheckTimer->setInterval(60000);

    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::autoRefresh);
    connect(sourceCycleTimer, &QTimer::timeout, this, &MainWindow::showNextFeed);
    connect(magazineCheckTimer, &QTimer::timeout, this, &MainWindow::checkMagazineTimes);

    refreshTimer->start();
    magazineCheckTimer->start();

    if (retentionDays > 0) {
        dbManager->deleteOldNews(retentionDays);
    }

    statusLabel->setText(tr("✅ Ready – Last update: %1")
                         .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
}

MainWindow::~MainWindow()
{
    if (refreshTimer) {
        refreshTimer->stop();
        delete refreshTimer;
    }
    if (sourceCycleTimer) {
        sourceCycleTimer->stop();
        delete sourceCycleTimer;
    }
    if (magazineCheckTimer) {
        magazineCheckTimer->stop();
        delete magazineCheckTimer;
    }
}

// ------ دالة مساعدة لإنشاء الأزرار ------
QPushButton* MainWindow::createButton(const QString &text, const QString &color, const QString &hover, const QString &icon)
{
    QPushButton *btn = new QPushButton(text);
    btn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            padding: 5px 10px;
            font-weight: bold;
            border-radius: 4px;
            border: none;
            font-size: 12px;
            min-width: 60px;
        }
        QPushButton:hover {
            background-color: %2;
        }
        QPushButton:pressed {
            background-color: %3;
        }
    )").arg(color, hover, color));
    if (!icon.isEmpty()) {
        btn->setText(icon + " " + text);
    }
    return btn;
}

// ------ بناء الواجهة ------
void MainWindow::initUI()
{
    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ===== شريط العنوان =====    // ===== Title bar =====
    titleBar = new QWidget(this);
    titleBar->setFixedHeight(70);
    titleBar->setStyleSheet(R"(
        QWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #2c3e50, stop:0.5 #1a5276, stop:1 #2c3e50);
        }
    )");
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(20, 10, 20, 10);
    titleLayout->setSpacing(0);

    QLabel *titleLabel = new QLabel(tr("RSS Reader - Advanced Electronic Newspaper"), titleBar);
    titleLabel->setStyleSheet("color: white; font-size: 20px; font-weight: bold;");
    titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    titleLayout->addWidget(titleLabel);

    // مسافة مرنة تدفع العنوانين للأطراف
    titleLayout->addStretch();

    QLabel *subtitleLabel = new QLabel(tr("Latest news from multiple sources - Smart daily magazine"), titleBar);
    subtitleLabel->setStyleSheet("color: #ecf0f1; font-size: 13px;");
    subtitleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    subtitleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    titleLayout->addWidget(subtitleLabel);

    mainLayout->addWidget(titleBar);

    // ===== رأس المجلة (مخفي) =====
    magazineHeader = new QWidget(this);
    magazineHeader->setFixedHeight(70);
    magazineHeader->setStyleSheet(R"(
        QWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #8e44ad, stop:0.5 #9b59b6, stop:1 #8e44ad);
        }
    )");
    magazineHeader->setVisible(false);
    QHBoxLayout *magazineHeaderLayout = new QHBoxLayout(magazineHeader);
    magazineHeaderLayout->setContentsMargins(20, 10, 20, 10);
    QLabel *magazineTitleLabel = new QLabel(tr("📖 Today's Magazine"), magazineHeader);
    magazineTitleLabel->setStyleSheet("color: white; font-size: 24px; font-weight: bold;");
    magazineHeaderLayout->addWidget(magazineTitleLabel);

    magazineHeaderLayout->addStretch();

    QLabel *magazineCountLabel = new QLabel("", magazineHeader);
    magazineCountLabel->setObjectName("magazineCountLabel");
    magazineCountLabel->setStyleSheet(
        "color: #e8f5e9; font-size: 16px; "
        "background: rgba(255,255,255,0.15); "
        "padding: 5px 15px; border-radius: 15px;"
    );
    magazineCountLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    magazineHeaderLayout->addWidget(magazineCountLabel);
    mainLayout->addWidget(magazineHeader);

    // ===== شريط الأخبار =====
    tickerContainer = new QWidget(this);
    QVBoxLayout *tickerLayout = new QVBoxLayout(tickerContainer);
    tickerLayout->setContentsMargins(0, 0, 0, 0);
    ticker = new NewsTicker(tickerContainer);
    tickerLayout->addWidget(ticker);
    mainLayout->addWidget(tickerContainer);

    // ===== منطقة الأخبار =====
    stackedNews = new QStackedWidget(this);
    stackedNews->setStyleSheet("QStackedWidget { background-color: #ecf0f1; }");
    newsListPage = new QWidget(stackedNews);
    QVBoxLayout *listLayout = new QVBoxLayout(newsListPage);
    listLayout->setContentsMargins(0, 0, 0, 0);
    newsScroll = new QScrollArea(newsListPage);
    newsScroll->setWidgetResizable(true);
    newsScroll->setStyleSheet(R"(
        QScrollArea { border: none; background-color: #ecf0f1; }
        QScrollBar:vertical { width: 10px; background: #f1f1f1; border-radius: 5px; }
        QScrollBar::handle:vertical { background: #bdc3c7; border-radius: 5px; min-height: 20px; }
        QScrollBar::handle:vertical:hover { background: #95a5a6; }
    )");
    QWidget *container = new QWidget(newsScroll);
    newsLayout = new QVBoxLayout(container);
    newsLayout->setAlignment(Qt::AlignTop);
    newsLayout->setSpacing(15);
    newsLayout->setContentsMargins(15, 15, 15, 15);
    container->setLayout(newsLayout);
    newsScroll->setWidget(container);
    listLayout->addWidget(newsScroll);
    stackedNews->addWidget(newsListPage);
    magazinePage = new MagazinePage(stackedNews);
    stackedNews->addWidget(magazinePage);
    mainLayout->addWidget(stackedNews);

    // ===== شريط التحكم السفلي =====
    QHBoxLayout *bottomBar = new QHBoxLayout();
    bottomBar->setContentsMargins(8, 6, 8, 6);
    bottomBar->setSpacing(6);

    // 1. قائمة المصادر
    sourceCombo = new QComboBox(this);
    sourceCombo->setMinimumHeight(30);
    sourceCombo->setStyleSheet(R"(
        QComboBox {
            padding: 3px 8px; min-width: 150px; font-size: 12px;
            border: 1px solid #bdc3c7; border-radius: 4px; background: white;
        }
    )");
    connect(sourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSourceChanged);
    bottomBar->addWidget(sourceCombo);

    // 2. السابق / التالي
    prevBtn = createButton(tr(""), "#3498db", "#2980b9", "◀");
    connect(prevBtn, &QPushButton::clicked, this, &MainWindow::showPrevFeed);
    bottomBar->addWidget(prevBtn);
    nextBtn = createButton(tr(""), "#3498db", "#2980b9", "▶");
    connect(nextBtn, &QPushButton::clicked, this, &MainWindow::showNextFeed);
    bottomBar->addWidget(nextBtn);

    // 3. تحديث
    refreshBtn = createButton(tr("Refresh"), "#2ecc71", "#27ae60", "🔄");
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::loadFeeds);
    bottomBar->addWidget(refreshBtn);

    // 4. إضافة / حذف
    addSourceBtn = createButton(tr("Add"), "#27ae60", "#2ecc71", "➕");
    connect(addSourceBtn, &QPushButton::clicked, this, &MainWindow::showAddSourceDialog);
    bottomBar->addWidget(addSourceBtn);
    delSourceBtn = createButton(tr("Delete"), "#e74c3c", "#c0392b", "➖");
    connect(delSourceBtn, &QPushButton::clicked, this, &MainWindow::deleteCurrentSource);
    bottomBar->addWidget(delSourceBtn);

    // 5. المجلة
    magazineBtn = createButton(tr("Magazine"), "#e67e22", "#f39c12", "📖");
    connect(magazineBtn, &QPushButton::clicked, this, &MainWindow::switchToMagazineMode);
    bottomBar->addWidget(magazineBtn);

    // 6. زر المزيد
    moreBtn = new QPushButton("⋯");
    moreBtn->setFixedSize(36, 36);
    moreBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #7f8c8d; color: white; font-weight: bold;
            border-radius: 4px; font-size: 16px;
        }
        QPushButton:hover { background-color: #6c7a7d; }
    )");
    moreMenu = new QMenu(this);
    moreMenu->setStyleSheet("QMenu { font-size: 12px; }");

    QAction *aboutAction = moreMenu->addAction(tr("About"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
    
    QAction *saveAction = moreMenu->addAction(tr("Save"));
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveCurrentNews);

    QAction *archiveAction = moreMenu->addAction(tr("Archive"));
    connect(archiveAction, &QAction::triggered, this, &MainWindow::showArchiveDialog);

    QAction *yesterdayAction = moreMenu->addAction(tr("Yesterday"));
    connect(yesterdayAction, &QAction::triggered, this, &MainWindow::showYesterdayNews);

    QAction *scheduleAction = moreMenu->addAction(tr("⏰ Schedule"));
    connect(scheduleAction, &QAction::triggered, this, &MainWindow::showSettingsDialog);

  
    // (اختياري) زر مسح التراكم – مفيد إذا أردت تفريغ القائمة يدوياً
    QAction *clearAction = moreMenu->addAction(tr("Clear Accumulation"));
    connect(clearAction, &QAction::triggered, this, [this]() {
        sourceNewsMap.clear();
        allDisplayedNews.clear();
        clearNewsLayout();
        statusLabel->setText(tr("🗑 Accumulation cleared"));
  
    });

    moreBtn->setMenu(moreMenu);
    bottomBar->addWidget(moreBtn);

    // statusLabel
    statusLabel = new QLabel(tr("Ready"));
   // مؤشر الحالة (ضوء)
    statusIndicator = new QLabel();
    statusIndicator->setFixedSize(16, 16);
    statusIndicator->setStyleSheet("background-color: #95a5a6; border-radius: 8px;"); // رمادي افتراضي
    bottomBar->addWidget(statusIndicator);
    statusLabel->setStyleSheet("color: #2c3e50; font-size: 12px; padding: 5px;");
    bottomBar->addWidget(statusLabel);
    bottomBar->addStretch();

    // زر العودة للقائمة
    QPushButton *backToListBtn = createButton(tr("List"), "#95a5a6", "#7f8c8d", "↩");
    connect(backToListBtn, &QPushButton::clicked, [this]() { showNormalMode(); });
    bottomBar->addWidget(backToListBtn);

    mainLayout->addLayout(bottomBar);
    setCentralWidget(central);

    // قائمة السياق
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QMainWindow::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu menu(this);
        QAction *refreshAction = menu.addAction(tr("🔄 Refresh"));
        connect(refreshAction, &QAction::triggered, this, &MainWindow::loadFeeds);
        QAction *archiveAction = menu.addAction(tr("📂 View Archive"));
        connect(archiveAction, &QAction::triggered, this, &MainWindow::showArchiveDialog);
        QAction *magazineAction = menu.addAction(tr("📖 View Magazine"));
        connect(magazineAction, &QAction::triggered, this, &MainWindow::switchToMagazineMode);
        menu.exec(mapToGlobal(pos));
    });
    qDebug() << "✅ تم تهيئة الواجهة بنجاح";
}

// ------ جلب وعرض الأخبار (النسخة المعدلة بالتراكم الدوري الذكي) ------
void MainWindow::loadFeeds()
{
    if (sources.isEmpty()) {
        showMessage(tr("📌 No RSS sources found."), "#e74c3c", true);
        setStatusIndicator("#e74c3c");
        return;
    }
    if (sourceCycleTimer) sourceCycleTimer->start(30000);

    QString sourceUrl = sources[currentSourceIndex].second;
    QString sourceName = sources[currentSourceIndex].first;
    setStatusIndicator("#f1c40f"); // أصفر
    statusLabel->setText(tr("🔄 Loading news..."));
    qDebug() << "🔄 بدء جلب المصدر:" << sourceName;
    fetcher->fetchFeed(sourceUrl, sourceName);
}


void MainWindow::setStatusIndicator(const QString &color)
{
    if (!statusIndicator) return;
    statusIndicator->setStyleSheet(QString("background-color: %1; border-radius: 8px;").arg(color));
}

// ========== التعديل الجوهري: onFeedFetched بالتراكم الدوري ==========
void MainWindow::onFeedFetched(const QString &sourceName, const QList<NewsItem> &articles)
{
    qDebug() << "📩 استلام" << articles.size() << "خبراً من" << sourceName;
    if (articles.isEmpty()) return;

    // 1. استبدال أخبار هذا المصدر (وليس إضافتها فوق القديمة)
    sourceNewsMap[sourceName] = articles;  // يحل محل القديم تلقائياً

    // 2. إعادة بناء القائمة المعروضة من كل المصادر
    allDisplayedNews.clear();
    // نرتب المصادر حسب ترتيب ظهورها في القائمة المنسدلة (sourceCombo)
    for (int i = 0; i < sourceCombo->count(); ++i) {
        QString srcName = sourceCombo->itemText(i);
        if (sourceNewsMap.contains(srcName)) {
            allDisplayedNews.append(sourceNewsMap[srcName]);
        }
    }

    // 3. عرض القائمة الجديدة (مسح وإعادة بناء)
    clearNewsLayout();
    // حد أقصى 200 خبر للحفاظ على الأداء (يمكن تعديله)
    int count = qMin(allDisplayedNews.size(), 200);
    for (int i = 0; i < count; ++i) {
        addNewsItem(allDisplayedNews[i]);
    }

    // 4. تحديث شريط الأخبار بكل العناوين من كل المصادر
    QStringList allTitles, allSources, allDates;
    for (const NewsItem &item : allDisplayedNews) {
        allTitles << item.title;
        allSources << item.source;
        allDates << item.pubDate;
    }
    ticker->updateTicker(allTitles, allSources, allDates);

    forceUpdateUI();
    setStatusIndicator("#2ecc71"); // أخضر
    statusLabel->setText(tr("📰 Total %1 articles from %2 sources")
                         .arg(allDisplayedNews.size())
                         .arg(sourceNewsMap.size()));

    // 5. الحفظ التلقائي (إذا كان مفعلاً)
    if (autoSaveEnabled && todaySavedCount < dailySaveLimit) {
        int remaining = dailySaveLimit - todaySavedCount;
        int savedNow = 0;
        for (int i = 0; i < articles.size() && savedNow < remaining; ++i) {
            const NewsItem &item = articles[i];
            if (dbManager->saveArticle(item)) {
                savedNow++;
            }
        }
        todaySavedCount += savedNow;
        QSettings settings("MyCompany", "RSSReader");
        settings.setValue("todaySavedCount", todaySavedCount);
        settings.setValue("lastSaveDate", QDate::currentDate().toString(Qt::ISODate));
        if (savedNow > 0) {
            statusLabel->setText(tr("💾 Auto-save: %1 new articles (daily total: %2/%3)")
                                 .arg(savedNow).arg(todaySavedCount).arg(dailySaveLimit));
        }
    }
}

void MainWindow::onFetchError(const QString &sourceName, const QString &error)
{
    qDebug() << "❌ خطأ في" << sourceName << ":" << error;
    setStatusIndicator("#e74c3c"); // أحمر
    statusLabel->setText(tr("❌ Error: %1").arg(error));
    // عند حدوث خطأ، لا نحذف الأخبار القديمة لهذا المصدر، بل نتركها كما هي.
    // لكن يمكن إضافة رسالة للمستخدم.
}

void MainWindow::onAllFeedsFetched(const QList<NewsItem> &allArticles)
{
    qDebug() << "📦 اكتمال جلب جميع المصادر - إجمالي الأخبار:" << allArticles.size();
    // هذا الإشارة تُستخدم لوضع المجلة، لكننا الآن نعتمد على onFeedFetched للتراكم.
    // نحدّث groupedArticles للاستخدام في وضع المجلة.
    groupedArticles = groupSimilarNews(allArticles);
    if (stackedNews->currentIndex() == 1) {
        magazinePage->buildMagazine(groupedArticles);
    }
    statusLabel->setText(tr("✅ Fetched %1 articles, grouped into %2 unique stories")
                         .arg(allArticles.size()).arg(groupedArticles.size()));
}

// ------ باقي الدوال (لم تتغير) ------
void MainWindow::clearNewsLayout()
{
    while (newsLayout->count()) {
        QLayoutItem *item = newsLayout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    newsLayout->invalidate();
    newsLayout->update();
}

void MainWindow::showMessage(const QString &text, const QString &color, bool isError)
{
    QLabel *label = new QLabel(text);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QString(R"(
        QLabel {
            color: %1;
            padding: 30px;
            font-size: 16px;
            background-color: %2;
            border-radius: 10px;
        }
    )").arg(isError ? "#e74c3c" : color,
            isError ? "#fde8e8" : "#f8f9fa"));
    newsLayout->addWidget(label);
}

void MainWindow::addNewsItem(const NewsItem &item, bool isArchived)
{
    QFrame *frame = new QFrame();
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setStyleSheet(R"(
        QFrame {
            background-color: white;
            border-radius: 10px;
            border: 1px solid #e0e0e0;
            padding: 5px;
        }
        QFrame:hover {
            border: 2px solid #3498db;
            background-color: #f8f9fa;
        }
    )");

    QHBoxLayout *mainLayout = new QHBoxLayout(frame);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(15);

    // الأيقونة
    QVBoxLayout *iconLayout = new QVBoxLayout();
    iconLayout->setAlignment(Qt::AlignTop);
    iconLayout->setContentsMargins(0, 0, 0, 0);

    // ----- استخدام اللغة الصحيحة -----
    QString langCode = detectLanguage(item.title + " " + item.description);

    IconManager *iconMgr = IconManager::instance();
    QPixmap iconPixmap = iconMgr->getIconForNews(item.title, item.description, langCode, 48);

    QLabel *iconLabel = new QLabel();
    iconLabel->setPixmap(iconPixmap);
    iconLabel->setFixedSize(48, 48);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet(R"(
        QLabel {
            background: rgba(52, 152, 219, 0.1);
            border-radius: 8px;
            padding: 4px;
        }
    )");
    iconLayout->addWidget(iconLabel);

    QString category = iconMgr->classifyNews(item.title, item.description, langCode);
    QLabel *categoryLabel = new QLabel(tr(category.toUtf8().constData()));
    categoryLabel->setAlignment(Qt::AlignCenter);
    categoryLabel->setStyleSheet(QString(R"(
        QLabel {
            font-size: 10px;
            color: white;
            background-color: %1;
            padding: 2px 8px;
            border-radius: 10px;
            font-weight: bold;
        }
    )").arg(getCategoryColor(category)));
    categoryLabel->setFixedHeight(20);
    iconLayout->addWidget(categoryLabel);

    mainLayout->addLayout(iconLayout);

    // المحتوى
    QVBoxLayout *contentLayout = new QVBoxLayout();
    contentLayout->setSpacing(6);

    QLabel *title = new QLabel(item.title);
    title->setStyleSheet(R"(
        QLabel {
            color: #000000;
            font-size: 16px;
            font-weight: bold;
        }
    )");
    title->setWordWrap(true);
    title->setTextInteractionFlags(Qt::TextSelectableByMouse);
    contentLayout->addWidget(title);

 // ---- وصف الخبر (عرض كامل بدون أزرار أو سكرول) ----
    QLabel *descLabel = new QLabel(item.description);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet(R"(
        QLabel {
            font-size: 14px;
            color: #2c2c2c;
            padding: 0px;
        }
    )");
    descLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    descLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    contentLayout->addWidget(descLabel);

    // المصدر (رابط قابل للنقر)
    QLabel *info = new QLabel();
    info->setTextFormat(Qt::RichText);
    QString sourceDisplay = isArchived ? item.source : item.source;
    QString sourceLink = QString("<a href=\"%1\" style=\"color: #3498db; text-decoration: none;\">📌 %2</a> <span style=\"color: #95a5a6;\">| 🕒 %3</span>")
                         .arg(item.link)
                         .arg(sourceDisplay)
                         .arg(item.pubDate);
    info->setText(sourceLink);
    info->setOpenExternalLinks(true);
    info->setStyleSheet("font-size: 12px; font-weight: bold;");
    contentLayout->addWidget(info);

    mainLayout->addLayout(contentLayout, 1);

    newsLayout->addWidget(frame);
}
// (ملاحظة: دالة setupTicker أصبحت غير مستخدمة، لأننا نستخدم ticker->updateTicker مباشرة)
// يمكن حذفها، لكن نتركها احتياطاً.
void MainWindow::setupTicker(const QStringList &titles, const QStringList &sources, const QStringList &dates)
{
    if (titles.isEmpty()) return;
    ticker->updateTicker(titles, sources, dates);
}

void MainWindow::forceUpdateUI()
{
    newsLayout->update();
    newsLayout->activate();

    QWidget *container = newsScroll->widget();
    if (container) {
        container->update();
        container->adjustSize();
    }

    newsScroll->update();
    newsScroll->viewport()->update();
    QApplication::processEvents();
}

void MainWindow::debugNewsLayout()
{
    qDebug() << "🔍 عدد العناصر في newsLayout:" << newsLayout->count();
    qDebug() << "🔍 newsScroll مرئي:" << newsScroll->isVisible();
    qDebug() << "🔍 stackedNews currentIndex:" << stackedNews->currentIndex();
}

QString MainWindow::getCategoryColor(const QString &category) const
{
    static QMap<QString, QString> colors = {
        {"politics", "#e74c3c"},
        {"sports", "#2ecc71"},
        {"health", "#1abc9c"},
        {"economy", "#f39c12"},
        {"tech", "#3498db"},
        {"military", "#2c3e50"},
        {"environment", "#27ae60"},
        {"culture", "#9b59b6"},
        {"default", "#95a5a6"}
    };
    return colors.value(category, "#95a5a6");
}

// ===== إدارة المصادر =====
void MainWindow::updateSourceCombo()
{
    sourceCombo->clear();
    for (const auto &pair : sources) {
        sourceCombo->addItem(pair.first, pair.second);
    }
}

void MainWindow::onSourceChanged(int index)
{
    if (index >= 0 && index < sources.size()) {
        currentSourceIndex = index;
        loadFeeds();
    }
}

void MainWindow::showAddSourceDialog()
{
    bool ok1, ok2;
    QString name = QInputDialog::getText(this, tr("➕ Add Source"),
                                        tr("Source name:"), QLineEdit::Normal, "", &ok1);
    if (!ok1 || name.isEmpty()) return;

    QString url = QInputDialog::getText(this, tr("➕ Add Source"),
                                       tr("RSS URL:"), QLineEdit::Normal, "", &ok2);
    if (!ok2 || url.isEmpty()) return;

    if (dbManager->addSource(name, url)) {
        sources = dbManager->getSources();
        updateSourceCombo();
        QMessageBox::information(this, tr("✅ Added successfully"),
                                tr("Source '%1' added successfully.").arg(name));
        loadFeeds();
    } else {
        QMessageBox::warning(this, tr("❌ Error"), tr("Failed to add source. The URL may already exist."));
    }
}

void MainWindow::deleteCurrentSource()
{
    if (sources.isEmpty()) return;

    int idx = sourceCombo->currentIndex();
    if (idx < 0 || idx >= sources.size()) return;

    QString url = sources[idx].second;
    QString name = sources[idx].first;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("⚠️ Confirm deletion"),
        tr("Are you sure you want to delete source '%1'?").arg(name),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (dbManager->deleteSource(url)) {
            sources = dbManager->getSources();
            updateSourceCombo();
            // حذف المصدر من الخريطة أيضاً
            sourceNewsMap.remove(name);
            // إعادة بناء القائمة المعروضة
            allDisplayedNews.clear();
            for (int i = 0; i < sourceCombo->count(); ++i) {
                QString srcName = sourceCombo->itemText(i);
                if (sourceNewsMap.contains(srcName)) {
                    allDisplayedNews.append(sourceNewsMap[srcName]);
                }
            }
            clearNewsLayout();
            for (int i = 0; i < allDisplayedNews.size(); ++i) {
                addNewsItem(allDisplayedNews[i]);
            }
            if (!sources.isEmpty()) {
                currentSourceIndex = qMin(currentSourceIndex, sources.size() - 1);
                loadFeeds();
            } else {
                clearNewsLayout();
                showMessage(tr("📌 No RSS sources found. Please add a source."), "#e74c3c", true);
            }
            QMessageBox::information(this, tr("✅ Deleted successfully"), tr("Source deleted successfully."));
        } else {
            QMessageBox::warning(this, tr("❌ Error"), tr("Failed to delete source."));
        }
    }
}

void MainWindow::showPrevFeed()
{
    if (sources.isEmpty()) return;
    currentSourceIndex = (currentSourceIndex - 1 + sources.size()) % sources.size();
    sourceCombo->setCurrentIndex(currentSourceIndex);
}

void MainWindow::showNextFeed()
{
    if (sources.isEmpty()) return;
    currentSourceIndex = (currentSourceIndex + 1) % sources.size();
    sourceCombo->setCurrentIndex(currentSourceIndex);
}

// ===== الأرشيف =====
void MainWindow::showArchiveDialog()
{
    QStringList dates = dbManager->getDistinctDates();
    if (dates.isEmpty()) {
        QMessageBox::information(this, tr("Archive"), tr("No news saved yet."));
        return;
    }
    QDialog dlg(this);
    dlg.setWindowTitle(tr("📂 Archive - Choose a date"));
    dlg.setFixedSize(250, 350);
    QVBoxLayout *dlgLayout = new QVBoxLayout(&dlg);
    QListWidget *listWidget = new QListWidget(&dlg);
    listWidget->addItems(dates);
    dlgLayout->addWidget(listWidget);
    QPushButton *cancelBtn = new QPushButton(tr("Cancel"), &dlg);
    dlgLayout->addWidget(cancelBtn);
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(listWidget, &QListWidget::itemClicked, [&](QListWidgetItem *item) {
        QString dateStr = item->text();
        QDate date = QDate::fromString(dateStr, Qt::ISODate);
        if (!date.isValid()) return;
        QList<NewsItem> archived = dbManager->getNewsByDate(date);
        if (archived.isEmpty()) {
            QMessageBox::information(this, tr("Archive"), tr("No news for this date."));
            return;
        }
        magazinePage->buildMagazine(archived);
        updateMagazineHeader(archived.size());
        showMagazineMode();
        dlg.accept();
    });
    dlg.exec();
}

void MainWindow::showYesterdayNews()
{
    QDate yesterday = QDate::currentDate().addDays(-1);
    QList<NewsItem> archived = dbManager->getNewsByDate(yesterday);
    if (archived.isEmpty()) {
        QMessageBox::information(this, tr("Yesterday"), tr("No news saved for yesterday."));
        return;
    }
    magazinePage->buildMagazine(archived);
    updateMagazineHeader(archived.size());
    showMagazineMode();
    statusLabel->setText(tr("📰 Yesterday's paper - %1 articles").arg(archived.size()));
}

void MainWindow::saveCurrentNews()
{
    if (currentArticles.isEmpty()) {
        QMessageBox::warning(this, tr("⚠️ Warning"), tr("No news to save."));
        return;
    }

    int count = qMin(currentArticles.size(), 20);
    int saved = 0;

    for (int i = 0; i < count; ++i) {
        const NewsItem &item = currentArticles[i];
        if (dbManager->saveArticle(item)) {
            saved++;
        }
    }

    QMessageBox::information(this, tr("✅ Saved"),
                            tr("Saved %1 articles to the archive.").arg(saved));
    statusLabel->setText(tr("💾 Saved %1 articles").arg(saved));
}

// ------ المجلة والجدولة ------
void MainWindow::autoRefresh()
{
    loadFeeds();
    statusLabel->setText(tr("🔄 Auto-refresh: %1")
                         .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
}

// ===== خوارزميات التجميع الذكي =====
double MainWindow::calculateSimilarity(const QString &s1, const QString &s2) const
{
    if (s1.isEmpty() || s2.isEmpty()) return 0.0;

    QStringList words1 = s1.split(' ', Qt::SkipEmptyParts);
    QStringList words2 = s2.split(' ', Qt::SkipEmptyParts);

    if (words1.isEmpty() || words2.isEmpty()) return 0.0;

    QSet<QString> set1(words1.begin(), words1.end());
    QSet<QString> set2(words2.begin(), words2.end());

    int intersection = set1.intersect(set2).size();
    int unionSize = set1.size() + set2.size() - intersection;

    return (unionSize == 0) ? 0.0 : (double)intersection / unionSize;
}

NewsItem MainWindow::selectRepresentative(const QList<NewsItem> &group) const
{
    if (group.isEmpty()) return NewsItem();
    if (group.size() == 1) return group.first();

    NewsItem best = group.first();

    for (const NewsItem &item : group) {
        if (item.description.length() > best.description.length()) {
            best = item;
        }
        else if (!item.imageUrl.isEmpty() && best.imageUrl.isEmpty()) {
            best = item;
        }
        else if (!item.pubDate.isEmpty() && !best.pubDate.isEmpty()) {
            QDate itemDate = parseDate(item.pubDate);
            QDate bestDate = parseDate(best.pubDate);
            if (itemDate.isValid() && bestDate.isValid() && itemDate > bestDate) {
                best = item;
            }
        }
    }

    return best;
}

QList<NewsItem> MainWindow::groupSimilarNews(const QList<NewsItem> &allNews) const
{
    if (allNews.isEmpty()) return QList<NewsItem>();

    QList<NewsItem> grouped;
    QList<NewsItem> remaining = allNews;

    while (!remaining.isEmpty()) {
        NewsItem representative = remaining.takeFirst();
        QList<NewsItem> group;
        group.append(representative);

        for (int i = remaining.size() - 1; i >= 0; --i) {
            double sim = calculateSimilarity(representative.title, remaining[i].title);
            if (sim > 0.65) {
                group.append(remaining.takeAt(i));
            }
        }

        NewsItem best = selectRepresentative(group);

        QStringList sourceList;
        for (const NewsItem &item : group) {
            if (!sourceList.contains(item.source)) {
                sourceList.append(item.source);
            }
        }
        best.source = sourceList.join("، ");

        grouped.append(best);
    }

    return grouped;
}

QString MainWindow::extractMainSource(const QString &sourceList) const
{
    QStringList sources = sourceList.split("، ");
    return sources.isEmpty() ? tr("Unknown source") : sources.first();
}

QString MainWindow::cleanHtml(const QString &html) const
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

QDate MainWindow::parseDate(const QString &dateStr) const
{
    QDate date;
    date = QDate::fromString(dateStr, "ddd, dd MMM yyyy");
    if (date.isValid()) return date;
    date = QDate::fromString(dateStr, Qt::ISODate);
    if (date.isValid()) return date;
    date = QDate::fromString(dateStr, "dd/MM/yyyy");
    if (date.isValid()) return date;
    return QDate();
}

// ===== المجلة والجدولة =====
void MainWindow::showSettingsDialog()
{
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        autoSaveEnabled = dlg.autoSaveEnabled();
        autoSaveCount = dlg.autoSaveCount();
        deduplicateOnSave = dlg.deduplicateOnSave();
        maxMagazineArticles = dlg.maxMagazineArticles();

        magazineTimes = dlg.magazineTimes();

        refreshTimer->setInterval(dlg.updateIntervalForeground() * 60 * 1000);
        sourceCycleTimer->setInterval(dlg.sourceCycleInterval() * 1000);

        dailySaveLimit = dlg.autoSaveCount();
        QSettings settings("MyCompany", "RSSReader");
        settings.setValue("dailySaveLimit", dailySaveLimit);
        applySettings();
    }
}

void MainWindow::applySettings()
{
    if (retentionDays > 0) {
        dbManager->deleteOldNews(retentionDays);
    }
}

void MainWindow::switchToMagazineMode()
{
    QList<NewsItem> magazineNews = groupedArticles;
    if (magazineNews.isEmpty()) {
        QDate today = QDate::currentDate();
        magazineNews = dbManager->getNewsByDate(today);
        if (magazineNews.isEmpty()) magazineNews = dbManager->getLatestNews(30);
        magazineNews = groupSimilarNews(magazineNews);
    }
    magazinePage->buildMagazine(magazineNews);
    updateMagazineHeader(magazineNews.size());
    showMagazineMode();
    statusLabel->setText(tr("📰 Magazine - %1 unique articles").arg(magazineNews.size()));
}

void MainWindow::updateMagazineHeader(int newsCount)
{
    if (!magazineHeader) return;

    QLabel *countLabel = magazineHeader->findChild<QLabel*>("magazineCountLabel");
    if (countLabel) {
        countLabel->setText(tr("%1 articles").arg(newsCount));
    }
}

void MainWindow::checkMagazineTimes()
{
    if (magazineTimes.isEmpty()) return;
    QDateTime now = QDateTime::currentDateTime();
    QTime currentTime = now.time();
    QStringList shownList = QSettings("MyCompany", "RSSReader").value("magazine_shown_today").toStringList();
    QSet<QString> shownToday(shownList.begin(), shownList.end());
    for (const QTime &t : magazineTimes) {
        if (currentTime >= t) {
            QString key = t.toString("HH:mm");
            if (!shownToday.contains(key)) {
                switchToMagazineMode();
                shownToday.insert(key);
                QSettings("MyCompany", "RSSReader").setValue("magazine_shown_today", QStringList(shownToday.begin(), shownToday.end()));
                break;
            }
        }
    }
}

// ===== أحداث النافذة =====
void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    static bool firstShow = true;
    if (firstShow) {
        firstShow = false;
        loadFeeds();
        if (sourceCycleTimer) sourceCycleTimer->start(30000);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) { QMainWindow::resizeEvent(event); }

void MainWindow::showNormalMode()
{
    if (titleBar) titleBar->setVisible(true);
    if (tickerContainer) tickerContainer->setVisible(true);
    if (magazineHeader) magazineHeader->setVisible(false);
    if (sourceCycleTimer) sourceCycleTimer->start(30000);
    stackedNews->setCurrentIndex(0);
    statusLabel->setText(tr("📋 Normal list view"));
    setStatusIndicator("#2ecc71"); // أخضر (القائمة العادية)
}

void MainWindow::showMagazineMode()
{
    if (titleBar) titleBar->setVisible(false);
    if (tickerContainer) tickerContainer->setVisible(false);
    if (magazineHeader) magazineHeader->setVisible(true);
    if (sourceCycleTimer) sourceCycleTimer->stop();
    stackedNews->setCurrentIndex(1);
    statusLabel->setText(tr("📖 View Magazine"));
    setStatusIndicator("#95a5a6"); // رمادي (وضع المجلة)
}

void MainWindow::showAboutDialog()
{
    QDialog aboutDialog(this);
    aboutDialog.setWindowTitle(tr("About"));
    aboutDialog.setFixedSize(520, 580);
    aboutDialog.setStyleSheet("QDialog { background-color: #f8f9fa; }");

    QVBoxLayout *layout = new QVBoxLayout(&aboutDialog);
    layout->setSpacing(12);
    layout->setContentsMargins(30, 25, 30, 25);

    // ===== أيقونة البرنامج =====
    QLabel *iconLabel = new QLabel();
    QPixmap iconPixmap = QIcon(":/icons/tray_icon.svg").pixmap(64, 64);
    iconLabel->setPixmap(iconPixmap);
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    // ===== عنوان البرنامج =====
    QLabel *titleLabel = new QLabel(tr("RSSReader"));
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #2c3e50;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    // ===== الإصدار =====
    QLabel *versionLabel = new QLabel(tr("Version 1.0.0"));
    versionLabel->setStyleSheet("font-size: 14px; color: #7f8c8d;");
    versionLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(versionLabel);

    // ===== وصف البرنامج =====
    QLabel *descLabel = new QLabel(tr(
        "An advanced, open-source RSS reader\n"
        "Supports multiple sources, smart daily magazine,\n"
        "Arabic interface with support for English and French translation."
    ));
    descLabel->setStyleSheet("font-size: 13px; color: #34495e;");
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);

    // ===== خط فاصل =====
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #ecf0f1; max-height: 1px;");
    layout->addWidget(line);

    // ===== معلومات المطور =====
    QLabel *authorLabel = new QLabel(tr("Developer: Samer Merhj"));
    authorLabel->setStyleSheet("font-size: 12px; color: #2c3e50;");
    authorLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(authorLabel);

    QLabel *emailLabel = new QLabel(tr("Email: mjosak7@gmail.com"));
    emailLabel->setStyleSheet("font-size: 12px; color: #7f8c8d;");
    emailLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(emailLabel);

    // ===== رابط المستودع =====
    QLabel *repoLabel = new QLabel();
    repoLabel->setTextFormat(Qt::RichText);
    repoLabel->setText(tr("<a href=\"https://github.com/samermerhj/RSSReader\" style=\"color: #3498db; text-decoration: none;\">📂 GitHub Repository</a>"));
    repoLabel->setOpenExternalLinks(true);
    repoLabel->setAlignment(Qt::AlignCenter);
    repoLabel->setStyleSheet("font-size: 12px;");
    layout->addWidget(repoLabel);

    // ===== خط فاصل =====
    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("background-color: #ecf0f1; max-height: 1px;");
    layout->addWidget(line2);

    // ===== الترخيص =====
    QLabel *licenseLabel = new QLabel(tr(
        "This software is free and open-source\n"
        "under the GNU General Public License v3.0"
    ));
    licenseLabel->setStyleSheet("font-size: 11px; color: #95a5a6;");
    licenseLabel->setAlignment(Qt::AlignCenter);
    licenseLabel->setWordWrap(true);
    layout->addWidget(licenseLabel);

    // ===== خط فاصل =====
    QFrame *line3 = new QFrame();
    line3->setFrameShape(QFrame::HLine);
    line3->setStyleSheet("background-color: #ecf0f1; max-height: 1px;");
    layout->addWidget(line3);

    // ===== قسم التبرع =====
    QLabel *donateTitle = new QLabel(tr("💝 Support the project"));
    donateTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50;");
    donateTitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(donateTitle);

    // زر التبرع عبر Coindrop
    QPushButton *donateBtn = new QPushButton(tr("☕ Donate via Coindrop"));
    donateBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #e74c3c;
            color: white;
            padding: 10px;
            border-radius: 8px;
            border: none;
            font-size: 15px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #c0392b; }
        QPushButton:pressed { background-color: #a93226; }
    )");
    connect(donateBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://coindrop.to/RSSReader"));
    });
    layout->addWidget(donateBtn);

    // عنوان Bitcoin
    QLabel *btcLabel = new QLabel(tr("Bitcoin Wallet Address:"));
    btcLabel->setStyleSheet("font-size: 12px; color: #7f8c8d;");
    btcLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(btcLabel);

    QString btcAddress = "bc1qrh4nw70w5hyrg4myuppv879z6sp40lzvr9k69m";
    QLabel *btcAddressLabel = new QLabel(btcAddress);
    btcAddressLabel->setStyleSheet(
        "font-size: 12px; "
        "color: #2c3e50; "
        "background-color: #ecf0f1; "
        "padding: 8px; "
        "border-radius: 4px; "
        "font-family: monospace;"
    );
    btcAddressLabel->setAlignment(Qt::AlignCenter);
    btcAddressLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(btcAddressLabel);

    // زر نسخ عنوان Bitcoin
    QPushButton *copyBtcBtn = new QPushButton(tr("📋 Copy address"));
    copyBtcBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3498db;
            color: white;
            padding: 6px 15px;
            border-radius: 6px;
            border: none;
            font-size: 12px;
        }
        QPushButton:hover { background-color: #2980b9; }
        QPushButton:pressed { background-color: #1f618d; }
    )");
    connect(copyBtcBtn, &QPushButton::clicked, [btcAddress]() {
        QApplication::clipboard()->setText(btcAddress);
        QMessageBox::information(nullptr,
            QObject::tr("Done"),
            QObject::tr("✅ Bitcoin address copied to clipboard!")
        );
    });
    layout->addWidget(copyBtcBtn, 0, Qt::AlignCenter);

    // ===== زر الإغلاق =====
    QPushButton *closeBtn = new QPushButton(tr("Close"));
    closeBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #95a5a6;
            color: white;
            padding: 8px 20px;
            border-radius: 6px;
            border: none;
            font-size: 13px;
        }
        QPushButton:hover { background-color: #7f8c8d; }
        QPushButton:pressed { background-color: #6c7a7d; }
    )");
    connect(closeBtn, &QPushButton::clicked, &aboutDialog, &QDialog::accept);
    layout->addWidget(closeBtn, 0, Qt::AlignCenter);

    aboutDialog.exec();
}
