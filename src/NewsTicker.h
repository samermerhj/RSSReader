#ifndef NEWSTICKER_H
#define NEWSTICKER_H

#include <QWidget>
#include <QLabel>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QStringList>
#include <QTimer>
#include <QMap>
#include <QVBoxLayout>    // ✅ تمت الإضافة
#include <QHBoxLayout>    // ✅ تمت الإضافة

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
class NewsTicker : public QWidget
{
    Q_OBJECT
public:
    explicit NewsTicker(QWidget *parent = nullptr);

    // تحديث الشريط بقائمة الأخبار والمصادر
    void updateTicker(const QStringList &titles, const QStringList &sources, 
                      const QStringList &pubDates = QStringList());
    
    // تعيين المصدر الحالي لعرض أخباره
    void setCurrentSource(const QString &sourceName);
    
    // الحصول على المصدر الحالي
    QString getCurrentSource() const { return currentSource; }
    
    // بدء/إيقاف التشغيل التلقائي
    void startAutoPlay();
    void stopAutoPlay();
    
    // الانتقال للخبر التالي/السابق يدوياً
    void nextNews();
    void prevNews();

signals:
    void sourceChanged(const QString &sourceName);  // عندما ينتقل لمصدر آخر
    void newsChanged(const QString &title);         // عند تغيير الخبر

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void nextNewsAuto();
    void onAnimationFinished();

private:
    // ------ عناصر الواجهة ------
    QLabel *titleLabel;        // عنوان الخبر
    QLabel *sourceLabel;       // المصدر + التاريخ
    QWidget *contentWidget;    // حاوية النص (للحركة)
    
    // ------ الرسوم المتحركة ------
    QPropertyAnimation *fadeOutAnim;
    QPropertyAnimation *fadeInAnim;
    QParallelAnimationGroup *animationGroup;
    bool isAnimating;
    
    // ------ البيانات ------
    QStringList titles;
    QStringList sources;
    QStringList pubDates;
    QString currentSource;
    int currentIndex;
    
    // ------ التشغيل التلقائي ------
    QTimer *autoPlayTimer;
    int autoPlayInterval;  // بالمللي ثانية
    
    // ------ دوال مساعدة ------
    void initUI();
    void updateContent(const QString &title, const QString &source, const QString &pubDate);
    void switchToNextSource();
    bool isNewsRepeated(const QString &title) const;
    QString getUniqueSource(const QString &currentTitle) const;
    QString extractMainSource(const QString &sourceList) const;
};

#endif // NEWSTICKER_H
