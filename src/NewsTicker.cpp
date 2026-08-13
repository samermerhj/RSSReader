#include "NewsTicker.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QFont>
#include <QDebug>
#include <QTimer>
#include <QResizeEvent>
#include <QGraphicsOpacityEffect>
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
NewsTicker::NewsTicker(QWidget *parent)
    : QWidget(parent)
    , currentIndex(0)
    , isAnimating(false)
    , autoPlayInterval(6500)
{
    initUI();
    
    autoPlayTimer = new QTimer(this);
    autoPlayTimer->setSingleShot(false);
    connect(autoPlayTimer, &QTimer::timeout, this, &NewsTicker::nextNewsAuto);
}

void NewsTicker::initUI()
{
    setFixedHeight(120);
    setAutoFillBackground(true);
    setAttribute(Qt::WA_StyledBackground, true);

    // 1. تم حذف border تماماً من هنا
    setStyleSheet(R"(
        QWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #1a6e3a,
                stop:0.3 #27ae60,
                stop:0.7 #2ecc71,
                stop:1 #1a6e3a);
            border-radius: 10px;
        }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 10, 20, 10);
    mainLayout->setSpacing(5);
    
    contentWidget = new QWidget(this);
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(contentWidget);
    effect->setOpacity(1.0);
    contentWidget->setGraphicsEffect(effect);
    contentWidget->setStyleSheet("background: transparent;");
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(3);
    
    // 2. titleLabel بدون padding
    titleLabel = new QLabel(tr("Welcome! Waiting for news..."), contentWidget);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setWordWrap(true);
    titleLabel->setStyleSheet(R"(
        QLabel {
            color: white;
            font-size: 26px;         
            font-weight: bold;
            background: transparent;
        }
    )");
    titleLabel->setMinimumHeight(50);
    contentLayout->addWidget(titleLabel);
    
    // 3. المصدر يُضاف مباشرة إلى contentLayout (بدون sourceLayout منفصل)
    sourceLabel = new QLabel("", contentWidget);
    sourceLabel->setAlignment(Qt::AlignCenter);
    sourceLabel->setStyleSheet(R"(
        QLabel {
            color: #d5f5e3;
            font-size: 9px;
            font-weight: bold;
            background: transparent;
        }
    )");
    
   
   QHBoxLayout *bottomLayout = new QHBoxLayout();
   bottomLayout->setContentsMargins(0, 0, 0, 0);
   bottomLayout->addWidget(sourceLabel);
   bottomLayout->addStretch();    // يدفع المصدر لليسار
   
   contentLayout->addLayout(bottomLayout);
    
    mainLayout->addWidget(contentWidget);
    
    // ---------- الحركات (لم يتم تغييرها) ----------
    fadeOutAnim = new QPropertyAnimation(effect, "opacity");
    fadeOutAnim->setDuration(500);
    fadeOutAnim->setStartValue(1.0);
    fadeOutAnim->setEndValue(0.0);
    fadeOutAnim->setEasingCurve(QEasingCurve::InOutQuad);
    
    fadeInAnim = new QPropertyAnimation(effect, "opacity");
    fadeInAnim->setDuration(500);
    fadeInAnim->setStartValue(0.0);
    fadeInAnim->setEndValue(1.0);
    fadeInAnim->setEasingCurve(QEasingCurve::InOutQuad);
    
    animationGroup = new QParallelAnimationGroup(this);
    animationGroup->addAnimation(fadeOutAnim);
    animationGroup->addAnimation(fadeInAnim);
    
    connect(fadeOutAnim, &QPropertyAnimation::finished, this, &NewsTicker::onAnimationFinished);
    
    updateContent(tr("Welcome! Loading news..."), tr("Please wait"), "");
}

void NewsTicker::updateTicker(const QStringList &titles, const QStringList &sources, 
                               const QStringList &pubDates)
{
    if (titles.isEmpty() || sources.isEmpty()) {
        updateContent(tr("📭 No news available"), tr("Please update sources"), "");
        return;
    }
    
    this->titles = titles;
    this->sources = sources;
    this->pubDates = pubDates;
    
    while (this->pubDates.size() < titles.size()) {
        this->pubDates.append("");
    }
    
    currentSource = extractMainSource(sources.first());
    currentIndex = 0;
    
    if (!titles.isEmpty()) {
        updateContent(titles[0], sources[0], pubDates[0]);
        startAutoPlay();
    }
}

void NewsTicker::setCurrentSource(const QString &sourceName)
{
    if (sourceName.isEmpty()) return;
    
    for (int i = 0; i < sources.size(); ++i) {
        if (sources[i].contains(sourceName)) {
            currentIndex = i;
            currentSource = sourceName;
            updateContent(titles[i], sources[i], pubDates[i]);
            break;
        }
    }
}

void NewsTicker::updateContent(const QString &title, const QString &source, const QString &pubDate)
{
    titleLabel->setText(title);
    
    QString sourceText = tr("📌 %1").arg(source);
    if (!pubDate.isEmpty()) {
        sourceText += tr("  |  🕒 %1").arg(pubDate);
    }
    sourceLabel->setText(sourceText);
}

void NewsTicker::nextNewsAuto()
{
    if (titles.isEmpty()) {
        // إذا لم تكن هناك عناوين، حاول إعادة التشغيل لاحقًا
        if (autoPlayTimer && !autoPlayTimer->isActive())
            autoPlayTimer->start(autoPlayInterval);
        return;
    }

    int nextIndex = (currentIndex + 1) % titles.size();

    // إذا تكرر الخبر، حاول الانتقال لمصدر آخر، وإذا فشل، تابع عادي
    if (isNewsRepeated(titles[nextIndex])) {
        switchToNextSource(); // قد يغير currentIndex
        // إذا لم يتغير شيء (لم يجد مصدراً آخر)، نستمر بالتسلسل العادي
        if (currentIndex == (nextIndex - 1 + titles.size()) % titles.size()) {
            // لم يتغير، نستخدم nextIndex الأصلي
        } else {
            // تم التغيير بنجاح، اخرج
            if (autoPlayTimer && !autoPlayTimer->isActive())
                autoPlayTimer->start(autoPlayInterval);
            return;
        }
    }

    // تنفيذ الحركة
    isAnimating = true;
    currentIndex = nextIndex; // إذا لم نغير شيئاً، استخدم nextIndex المحسوب
    currentSource = extractMainSource(sources[currentIndex]);
    fadeOutAnim->start();

    // تأكد من أن المؤقت التلقائي يعمل
    if (autoPlayTimer && !autoPlayTimer->isActive())
        autoPlayTimer->start(autoPlayInterval);
}
void NewsTicker::onAnimationFinished()
{
    if (fadeOutAnim->state() == QAbstractAnimation::Stopped) {
        if (currentIndex < titles.size()) {
            updateContent(titles[currentIndex], sources[currentIndex], pubDates[currentIndex]);
            emit newsChanged(titles[currentIndex]);
            emit sourceChanged(currentSource);
        }
        
        fadeInAnim->start();
    } else if (fadeInAnim->state() == QAbstractAnimation::Stopped) {
        isAnimating = false;
    }
}

void NewsTicker::nextNews()
{
    if (isAnimating || titles.isEmpty()) return;
    nextNewsAuto();
}

void NewsTicker::prevNews()
{
    if (isAnimating || titles.isEmpty()) return;
    
    int prevIndex = (currentIndex - 1 + titles.size()) % titles.size();
    
    isAnimating = true;
    currentIndex = prevIndex;
    currentSource = extractMainSource(sources[currentIndex]);
    
    fadeOutAnim->start();
}

void NewsTicker::switchToNextSource()
{
    QStringList uniqueSources;
    for (const QString &src : sources) {
        QString mainSource = extractMainSource(src);
        if (!uniqueSources.contains(mainSource)) {
            uniqueSources.append(mainSource);
        }
    }
    
    int currentSourceIndex = uniqueSources.indexOf(currentSource);
    if (currentSourceIndex == -1) {
        currentSourceIndex = 0;
    }
    
    int nextSourceIndex = (currentSourceIndex + 1) % uniqueSources.size();
    QString nextSource = uniqueSources[nextSourceIndex];
    
    for (int i = 0; i < sources.size(); ++i) {
        if (sources[i].contains(nextSource)) {
            currentIndex = i;
            currentSource = nextSource;
            
            isAnimating = true;
            fadeOutAnim->start();
            break;
        }
    }
}

bool NewsTicker::isNewsRepeated(const QString &title) const
{
    int checkCount = qMin(5, titles.size());
    for (int i = 1; i <= checkCount; ++i) {
        int checkIndex = (currentIndex - i + titles.size()) % titles.size();
        if (titles[checkIndex] == title) {
            return true;
        }
    }
    return false;
}

QString NewsTicker::extractMainSource(const QString &sourceList) const
{
    QStringList sources = sourceList.split("، ");
    return sources.isEmpty() ? tr("Unknown source") : sources.first();
}

QString NewsTicker::getUniqueSource(const QString &currentTitle) const
{
    for (int i = 0; i < sources.size(); ++i) {
        if (sources[i] != currentSource) {
            return sources[i];
        }
    }
    return currentSource;
}

void NewsTicker::startAutoPlay()
{
    if (autoPlayTimer) {
        autoPlayTimer->start(autoPlayInterval);
    }
}

void NewsTicker::stopAutoPlay()
{
    if (autoPlayTimer) {
        autoPlayTimer->stop();
    }
}

void NewsTicker::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    int width = event->size().width();
    if (width < 800) {
        titleLabel->setStyleSheet(R"(
            QLabel {
                color: white;
                font-size: 20px;
                font-weight: bold;
                background: transparent;
            }
        )");
    } else {
        titleLabel->setStyleSheet(R"(
            QLabel {
                color: white;
                font-size: 26px;
                font-weight: bold;
                background: transparent;
            }
        )");
    }
}
