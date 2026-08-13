#include "SettingsDialog.h"
#include "ResourceManager.h"  // 🔥 تمت الإضافة
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QListWidget>
#include <QTimeEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QFormLayout>
#include <QGroupBox>
#include <QStyle>
#include <QMessageBox>
#include <QLocale>

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

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    setMinimumSize(550, 650);
    resize(580, 680);
    initUI();
    loadSettings();
}

void SettingsDialog::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QTabWidget *tabs = new QTabWidget(this);
    mainLayout->addWidget(tabs);

    // ----- تبويب ١: الحفظ التلقائي -----
    QWidget *saveTab = new QWidget();
    QVBoxLayout *saveLayout = new QVBoxLayout(saveTab);

    autoSaveCheck = new QCheckBox(tr("Enable automatic news saving"));
    autoSaveCheck->setStyleSheet("font-weight: bold; font-size: 14px;");
    saveLayout->addWidget(autoSaveCheck);

    QFormLayout *saveForm = new QFormLayout();
    saveForm->setLabelAlignment(Qt::AlignLeft);

    autoSaveCountSpin = new QSpinBox();
    autoSaveCountSpin->setRange(5, 500);
    autoSaveCountSpin->setValue(100);
    autoSaveCountSpin->setSuffix(tr(" articles"));
    autoSaveCountSpin->setStyleSheet("QSpinBox { font-size: 13px; padding: 4px; }");
    saveForm->addRow(tr("Daily save limit:"), autoSaveCountSpin);

    retentionSpin = new QSpinBox();
    retentionSpin->setRange(1, 365);
    retentionSpin->setValue(30);
    retentionSpin->setSuffix(tr(" days"));
    retentionSpin->setStyleSheet("QSpinBox { font-size: 13px; padding: 4px; }");
    saveForm->addRow(tr("Keep news for:"), retentionSpin);

    saveLayout->addLayout(saveForm);
    saveLayout->addStretch();
    tabs->addTab(saveTab, QIcon::fromTheme("document-save"), tr("Save"));

    // ----- تبويب ٢: التصفية والعرض -----
    QWidget *filterTab = new QWidget();
    QVBoxLayout *filterLayout = new QVBoxLayout(filterTab);

    deduplicateCheck = new QCheckBox(tr("Filter similar news while saving"));
    deduplicateCheck->setStyleSheet("font-weight: bold; font-size: 14px;");
    filterLayout->addWidget(deduplicateCheck);

    QFormLayout *filterForm = new QFormLayout();
    filterForm->setLabelAlignment(Qt::AlignLeft);
    maxMagazineSpin = new QSpinBox();
    maxMagazineSpin->setRange(10, 200);
    maxMagazineSpin->setValue(30);
    maxMagazineSpin->setStyleSheet("QSpinBox { font-size: 13px; padding: 4px; }");
    filterForm->addRow(tr("Max articles in magazine:"), maxMagazineSpin);
    filterLayout->addLayout(filterForm);
    filterLayout->addStretch();
    tabs->addTab(filterTab, QIcon::fromTheme("view-filter"), tr("Filter"));

    // ----- تبويب ٣: تحديث الأخبار -----
    QWidget *updateTab = new QWidget();
    QVBoxLayout *updateLayout = new QVBoxLayout(updateTab);

    QFormLayout *updateForm = new QFormLayout();
    updateForm->setLabelAlignment(Qt::AlignLeft);

    foregroundCombo = new QComboBox();
    foregroundCombo->addItem(tr("Every 5 minutes"), 5);
    foregroundCombo->addItem(tr("Every 15 minutes"), 15);
    foregroundCombo->addItem(tr("Every 30 minutes"), 30);
    foregroundCombo->addItem(tr("Every hour"), 60);
    foregroundCombo->setCurrentIndex(2);
    foregroundCombo->setStyleSheet("QComboBox { font-size: 13px; padding: 4px; }");
    updateForm->addRow(tr("Update news (window open):"), foregroundCombo);

    backgroundCombo = new QComboBox();
    backgroundCombo->addItem(tr("Off"), 0);
    backgroundCombo->addItem(tr("Every 30 minutes"), 30);
    backgroundCombo->addItem(tr("Every hour"), 60);
    backgroundCombo->addItem(tr("Every 2 hours"), 120);
    backgroundCombo->setCurrentIndex(1);
    backgroundCombo->setStyleSheet("QComboBox { font-size: 13px; padding: 4px; }");
    updateForm->addRow(tr("Update news (background):"), backgroundCombo);

    cycleSpin = new QSpinBox();
    cycleSpin->setRange(10, 300);
    cycleSpin->setValue(30);
    cycleSpin->setSuffix(tr(" seconds"));
    cycleSpin->setStyleSheet("QSpinBox { font-size: 13px; padding: 4px; }");
    updateForm->addRow(tr("Auto-cycle sources every:"), cycleSpin);

    updateLayout->addLayout(updateForm);
    updateLayout->addStretch();
    tabs->addTab(updateTab, QIcon::fromTheme("view-refresh"), tr("Updates"));

    // ----- تبويب ٤: أوقات المجلة -----
    QWidget *magTab = new QWidget();
    QVBoxLayout *magLayout = new QVBoxLayout(magTab);

    QLabel *timeTitle = new QLabel(tr("Magazine display times:"));
    timeTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    magLayout->addWidget(timeTitle);

    timeListWidget = new QListWidget();
    timeListWidget->setStyleSheet("QListWidget { font-size: 13px; }");
    magLayout->addWidget(timeListWidget);

    QHBoxLayout *timeAddLayout = new QHBoxLayout();
    timeEdit = new QTimeEdit(QTime::currentTime());
    timeEdit->setDisplayFormat("HH:mm");
    timeEdit->setStyleSheet("QTimeEdit { font-size: 13px; padding: 4px; }");
    timeAddLayout->addWidget(timeEdit);

    QPushButton *addBtn = new QPushButton(QIcon::fromTheme("list-add"), tr("Add"));
    addBtn->setStyleSheet("QPushButton { padding: 5px 12px; font-size: 13px; }");
    connect(addBtn, &QPushButton::clicked, this, &SettingsDialog::addTime);
    timeAddLayout->addWidget(addBtn);

    QPushButton *removeBtn = new QPushButton(QIcon::fromTheme("list-remove"), tr("Remove selected"));
    removeBtn->setStyleSheet("QPushButton { padding: 5px 12px; font-size: 13px; }");
    connect(removeBtn, &QPushButton::clicked, this, &SettingsDialog::removeSelectedTime);
    timeAddLayout->addWidget(removeBtn);

    magLayout->addLayout(timeAddLayout);
    magLayout->addStretch();
    tabs->addTab(magTab, QIcon::fromTheme("clock"), tr("Magazine Times"));

    // ----- تبويب ٥: اللغة (جديد) -----
    QWidget *langTab = new QWidget();
    QVBoxLayout *langLayout = new QVBoxLayout(langTab);

    QLabel *langTitle = new QLabel(tr("Choose application language:"));
    langTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    langLayout->addWidget(langTitle);

    languageCombo = new QComboBox();
    languageCombo->addItem(tr("Arabic"), "ar");
    languageCombo->addItem("English", "en");
    languageCombo->addItem("Français", "fr");
    languageCombo->addItem("Русский", "ru");
    languageCombo->addItem("中文", "zh");
    languageCombo->setStyleSheet("QComboBox { font-size: 13px; padding: 4px; }");
    langLayout->addWidget(languageCombo);

    QLabel *noteLabel = new QLabel(tr("Note: The application must be restarted for the language change to take effect."));
    noteLabel->setWordWrap(true);
    noteLabel->setStyleSheet("color: #e67e22; font-size: 12px; padding: 10px;");
    langLayout->addWidget(noteLabel);

    langLayout->addStretch();
    tabs->addTab(langTab, QIcon::fromTheme("preferences-desktop-locale"), tr("Language"));

    // ----- أزرار موافق/تطبيق/إلغاء -----
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *applyBtn = new QPushButton(QIcon::fromTheme("dialog-ok-apply"), tr("Apply"));
    applyBtn->setStyleSheet("QPushButton { padding: 6px 16px; font-size: 13px; font-weight: bold; }");
    connect(applyBtn, &QPushButton::clicked, this, &SettingsDialog::applySettings);
    btnLayout->addWidget(applyBtn);

    QPushButton *okBtn = new QPushButton(QIcon::fromTheme("dialog-ok"), tr("OK"));
    okBtn->setStyleSheet("QPushButton { padding: 6px 16px; font-size: 13px; font-weight: bold; }");
    connect(okBtn, &QPushButton::clicked, this, [this]() {
        saveSettings();
        accept();
    });
    btnLayout->addWidget(okBtn);

    QPushButton *cancelBtn = new QPushButton(QIcon::fromTheme("dialog-cancel"), tr("Cancel"));
    cancelBtn->setStyleSheet("QPushButton { padding: 6px 16px; font-size: 13px; }");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    mainLayout->addLayout(btnLayout);
}

void SettingsDialog::applySettings()
{
    saveSettings();
}

void SettingsDialog::loadSettings()
{
    QSettings s("MyCompany", "RSSReader");
    autoSaveCheck->setChecked(s.value("autoSaveEnabled", false).toBool());
    autoSaveCountSpin->setValue(s.value("dailySaveLimit", 100).toInt());
    retentionSpin->setValue(s.value("retentionDays", 30).toInt());
    deduplicateCheck->setChecked(s.value("deduplicateOnSave", false).toBool());
    maxMagazineSpin->setValue(s.value("maxMagazineArticles", 30).toInt());

    int fg = s.value("updateIntervalForeground", 30).toInt();
    int idx = foregroundCombo->findData(fg);
    if (idx != -1) foregroundCombo->setCurrentIndex(idx);

    int bg = s.value("updateIntervalBackground", 30).toInt();
    idx = backgroundCombo->findData(bg);
    if (idx != -1) backgroundCombo->setCurrentIndex(idx);

    cycleSpin->setValue(s.value("sourceCycleInterval", 30).toInt());

    QStringList timesStr = s.value("magazine_times", QStringList()).toStringList();
    times.clear();
    for (const QString &t : timesStr) {
        QTime time = QTime::fromString(t, "HH:mm");
        if (time.isValid()) times.append(time);
    }
    timeListWidget->clear();
    for (const QTime &t : times)
        timeListWidget->addItem(t.toString("HH:mm"));

    // تحميل اللغة المحفوظة
    QString savedLang = s.value("language", "").toString();
    if (savedLang.isEmpty()) {
        savedLang = QLocale::system().name().left(2);
    }
    int langIdx = languageCombo->findData(savedLang);
    if (langIdx != -1) {
        languageCombo->setCurrentIndex(langIdx);
    } else {
        // الإنجليزية كخيار افتراضي إذا لم توجد اللغة
        langIdx = languageCombo->findData("en");
        if (langIdx != -1)
            languageCombo->setCurrentIndex(langIdx);
    }
}

void SettingsDialog::saveSettings()
{
    QSettings s("MyCompany", "RSSReader");
    s.setValue("autoSaveEnabled", autoSaveCheck->isChecked());
    s.setValue("dailySaveLimit", autoSaveCountSpin->value());
    s.setValue("retentionDays", retentionSpin->value());
    s.setValue("deduplicateOnSave", deduplicateCheck->isChecked());
    s.setValue("maxMagazineArticles", maxMagazineSpin->value());
    s.setValue("updateIntervalForeground", foregroundCombo->currentData().toInt());
    s.setValue("updateIntervalBackground", backgroundCombo->currentData().toInt());
    s.setValue("sourceCycleInterval", cycleSpin->value());

    QStringList timesStr;
    for (const QTime &t : times)
        timesStr << t.toString("HH:mm");
    s.setValue("magazine_times", timesStr);

    // حفظ اللغة المختارة
    QString selectedLang = languageCombo->currentData().toString();
    s.setValue("language", selectedLang);
}

void SettingsDialog::addTime()
{
    QTime t = timeEdit->time();
    if (!times.contains(t)) {
        times.append(t);
        timeListWidget->addItem(t.toString("HH:mm"));
    }
}

void SettingsDialog::removeSelectedTime()
{
    int row = timeListWidget->currentRow();
    if (row >= 0) {
        delete timeListWidget->takeItem(row);
        times.removeAt(row);
    }
}

bool SettingsDialog::autoSaveEnabled() const { return autoSaveCheck->isChecked(); }
int SettingsDialog::autoSaveCount() const { return autoSaveCountSpin->value(); }
int SettingsDialog::retentionDays() const { return retentionSpin->value(); }
bool SettingsDialog::deduplicateOnSave() const { return deduplicateCheck->isChecked(); }
int SettingsDialog::maxMagazineArticles() const { return maxMagazineSpin->value(); }
int SettingsDialog::updateIntervalForeground() const { return foregroundCombo->currentData().toInt(); }
int SettingsDialog::updateIntervalBackground() const { return backgroundCombo->currentData().toInt(); }
int SettingsDialog::sourceCycleInterval() const { return cycleSpin->value(); }
QList<QTime> SettingsDialog::magazineTimes() const { return times; }