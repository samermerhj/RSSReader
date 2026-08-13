#ifndef ICONMANAGER_H
#define ICONMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QStringList>
#include <QPixmap>
#include <QPair>
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
class IconManager : public QObject
{
    Q_OBJECT
public:
    static IconManager* instance();

    void loadIcons();
    void loadKeywordsFromJSON();   // ✅ أصبحت public لتُستدعى بعد التعلم

    QPixmap getIconForNews(const QString &title, const QString &description, const QString &langCode, int size = 32);
    QPixmap getDefaultIcon(int size = 32);
    QString classifyNews(const QString &title, const QString &description, const QString &langCode) const;
    QStringList getAllCategories() const;
    QString getCategoryColor(const QString &category) const;

    QList<QPair<QString, double>> getTopCategories(const QString &title, const QString &description, const QString &langCode) const;
    bool isConfident(const QList<QPair<QString, double>> &scores) const;

private:
    explicit IconManager(QObject *parent = nullptr);
    static IconManager *m_instance;

    QMap<QString, QMap<QString, QStringList>> keywordMap;
    QMap<QString, QMap<QString, double>> categoryWeights;
    QMap<QString, QString> categoryColors;

    void loadDefaultKeywords();
};

#endif // ICONMANAGER_H
