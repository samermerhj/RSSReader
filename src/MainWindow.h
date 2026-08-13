#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QStackedWidget>
#include <QSettings>
#include <QList>
#include <QPair>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QShowEvent>
#include <QLabel>
#include <QMap>
#include "DatabaseManager.h"
#include "NewsTicker.h"
#include "MagazinePage.h"
#include "RSSFetcher.h"
#include "SettingsDialog.h"
#include "IconManager.h"
#include "ImageDatabase.h"
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
class QComboBox;
class QPushButton;
class QVBoxLayout;
class QScrollArea;
class QFrame;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    // جلب الأخبار
    void loadFeeds();
    void onFeedFetched(const QString &sourceName, const QList<NewsItem> &articles);
    void onFetchError(const QString &sourceName, const QString &error);
    void onAllFeedsFetched(const QList<NewsItem> &allArticles);

    // إدارة المصادر
    void onSourceChanged(int index);
    void showAddSourceDialog();
    void deleteCurrentSource();
    void showPrevFeed();
    void showNextFeed();

    // الأرشيف والمجلة
    void showArchiveDialog();
    void showYesterdayNews();
    void saveCurrentNews();
    void showSettingsDialog();
    void switchToMagazineMode();
    void checkMagazineTimes();

    // أوضاع العرض
    void autoRefresh();
    void showNormalMode();
    void showMagazineMode();

    // الإعدادات
    void applySettings();
    // حول
    void showAboutDialog();

private:
    // ========== إدارة البيانات ==========
    DatabaseManager *dbManager;
    RSSFetcher *fetcher;
    QList<QPair<QString, QString>> sources;
    int currentSourceIndex;
    QList<NewsItem> currentArticles;
    QList<NewsItem> allFetchedArticles;
    QList<NewsItem> groupedArticles;

    // ========== نظام التراكم الذكي ==========
    QMap<QString, QList<NewsItem>> sourceNewsMap;   // آخر أخبار كل مصدر
    QList<NewsItem> allDisplayedNews;               // القائمة المعروضة

    // ========== مكونات الواجهة الرئيسية ==========
    QWidget *titleBar;
    QWidget *tickerContainer;
    QWidget *magazineHeader;
    QStackedWidget *stackedNews;
    QWidget *newsListPage;
    QScrollArea *newsScroll;
    QVBoxLayout *newsLayout;
    MagazinePage *magazinePage;
    NewsTicker *ticker;

    // ========== شريط التحكم السفلي ==========
    QComboBox *sourceCombo;
    QPushButton *prevBtn;
    QPushButton *nextBtn;
    QPushButton *refreshBtn;
    QPushButton *addSourceBtn;
    QPushButton *delSourceBtn;
    QPushButton *magazineBtn;
    QPushButton *moreBtn;
    QMenu *moreMenu;
    QLabel *statusLabel;
    QLabel *statusIndicator;    // مؤشر الحالة (ضوء)

    // ========== علبة النظام ==========
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;

    // ========== المؤقتات ==========
    QTimer *refreshTimer;
    QTimer *sourceCycleTimer;
    QTimer *magazineCheckTimer;
    QList<QTime> magazineTimes;

    // ========== إعدادات الحفظ التلقائي ==========
    bool autoSaveEnabled;
    int autoSaveCount;
    bool deduplicateOnSave;
    int maxMagazineArticles;
    int retentionDays;
    int dailySaveLimit;
    int todaySavedCount;
    QDate lastSaveDate;

    // ========== دوال مساعدة خاصة ==========
    void initUI();
    void updateSourceCombo();
    void clearNewsLayout();
    void showMessage(const QString &text, const QString &color = "#7f8c8d", bool isError = false);
    void addNewsItem(const NewsItem &item, bool isArchived = false);
    void forceUpdateUI();
    void debugNewsLayout();
    void updateMagazineHeader(int newsCount);
    QString getCategoryColor(const QString &category) const;
    QPushButton* createButton(const QString &text, const QString &color, const QString &hover, const QString &icon = "");

    // ========== دوال مؤشر الحالة ==========
    void setStatusIndicator(const QString &color);

    // ========== دوال شريط الأخبار ==========
    void setupTicker(const QList<NewsItem> &articles);
    void setupTicker(const QStringList &titles, const QStringList &sources, const QStringList &dates);

    // ========== خوارزميات التجميع الذكي ==========
    double calculateSimilarity(const QString &s1, const QString &s2) const;
    QList<NewsItem> groupSimilarNews(const QList<NewsItem> &allNews) const;
    NewsItem selectRepresentative(const QList<NewsItem> &group) const;
    QString extractMainSource(const QString &sourceList) const;

    // ========== دوال مساعدة عامة ==========
    QString cleanHtml(const QString &html) const;
    QDate parseDate(const QString &dateStr) const;
};

#endif // MAINWINDOW_H
