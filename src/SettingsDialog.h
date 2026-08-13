#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QList>
#include <QTime>
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
class QCheckBox;
class QSpinBox;
class QComboBox;
class QListWidget;
class QTimeEdit;
class QPushButton;
class QTabWidget;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    bool autoSaveEnabled() const;
    int autoSaveCount() const;
    int retentionDays() const;
    bool deduplicateOnSave() const;
    int maxMagazineArticles() const;
    int updateIntervalForeground() const;
    int updateIntervalBackground() const;
    int sourceCycleInterval() const;
    QList<QTime> magazineTimes() const;

private slots:
    void addTime();
    void removeSelectedTime();
    void applySettings();

private:
    void initUI();
    void loadSettings();
    void saveSettings();

    QCheckBox *autoSaveCheck;
    QSpinBox *autoSaveCountSpin;
    QSpinBox *retentionSpin;
    QCheckBox *deduplicateCheck;
    QSpinBox *maxMagazineSpin;
    QComboBox *foregroundCombo;
    QComboBox *backgroundCombo;
    QComboBox *languageCombo;  // <-- المتغير الجديد
    QSpinBox *cycleSpin;
    QListWidget *timeListWidget;
    QTimeEdit *timeEdit;
    QList<QTime> times;
};

#endif
