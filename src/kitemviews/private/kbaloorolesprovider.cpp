/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
 *   Copyright (C) 2013 by Vishesh Handa <me@vhanda.in>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "kbaloorolesprovider.h"

#include <KDebug>
#include <KGlobal>
#include <KLocale>

#include <baloo/file.h>

#include <QTime>
#include <QMap>

struct KBalooRolesProviderSingleton
{
    KBalooRolesProvider instance;
};
K_GLOBAL_STATIC(KBalooRolesProviderSingleton, s_balooRolesProvider)


KBalooRolesProvider& KBalooRolesProvider::instance()
{
    return s_balooRolesProvider->instance;
}

KBalooRolesProvider::~KBalooRolesProvider()
{
}

QSet<QByteArray> KBalooRolesProvider::roles() const
{
    return m_roles;
}

QHash<QByteArray, QVariant> KBalooRolesProvider::roleValues(const Baloo::File& file,
                                                            const QSet<QByteArray>& roles) const
{
    QHash<QByteArray, QVariant> values;

    int width = -1;
    int height = -1;

    QMapIterator<QString, QVariant> it(file.properties());
    while (it.hasNext()) {
        it.next();

        const QString property = it.key();
        const QByteArray role = roleForProperty(property);
        if (role.isEmpty() || !roles.contains(role)) {
            continue;
        }

        const QVariant value = it.value();

        if (role == "imageSize") {
            // Merge the two properties for width and height
            // as one string into the "imageSize" role
            if (property == QLatin1String("width")) {
                width = value.toInt();
            }
            else if (property == QLatin1String("height")) {
                height = value.toInt();
            }

            if (width >= 0 && height >= 0) {
                QString widthAndHeight = QString::number(width);
                widthAndHeight += QLatin1String(" x ");
                widthAndHeight += QString::number(height);
                values.insert(role, widthAndHeight);
            }
        } else if (role == "tags") {
            const QString tags = tagsFromValues(value.toStringList());
            values.insert(role, tags);
        } else if (role == "orientation") {
            const QString orientation = orientationFromValue(value.toInt());
            values.insert(role, orientation);
        } else if (role == "duration") {
            const QString duration = durationFromValue(value.toInt());
            values.insert(role, duration);
        } else {
            values.insert(role, value.toString());
        }
    }

    return values;
}

QByteArray KBalooRolesProvider::roleForProperty(const QString& property) const
{
    return m_roleForProperty.value(property);
}

KBalooRolesProvider::KBalooRolesProvider() :
    m_roles(),
    m_roleForProperty()
{
    struct PropertyInfo
    {
        const char* const property;
        const char* const role;
    };

    // Mapping from the URIs to the KFileItemModel roles. Note that this must not be
    // a 1:1 mapping: One role may contain several URI-values (e.g. the URIs for height and
    // width of an image are mapped to the role "imageSize")
    static const PropertyInfo propertyInfoList[] = {
        { "rating", "rating" },
        { "tag",        "tags" },
        { "comment",   "comment" },
        { "wordCount",     "wordCount" },
        { "lineCount",     "lineCount" },
        { "width",         "imageSize" },
        { "height",        "imageSize" },
        { "nexif.orientation", "orientation", },
        { "artist",     "artist" },
        { "album",    "album" },
        { "duration",      "duration" },
        { "trackNumber",   "track" }
        // { "http://www.semanticdesktop.org/ontologies/2010/04/30/ndo#copiedFrom",    "copiedFrom" }
    };

    for (unsigned int i = 0; i < sizeof(propertyInfoList) / sizeof(PropertyInfo); ++i) {
        m_roleForProperty.insert(propertyInfoList[i].property, propertyInfoList[i].role);
        m_roles.insert(propertyInfoList[i].role);
    }
}

QString KBalooRolesProvider::tagsFromValues(const QStringList& values) const
{
    QString tags;

    /*
    for (int i = 0; i < values.count(); ++i) {
        if (i > 0) {
            tags.append(QLatin1String(", "));
        }

        const ::Tag tag(values[i]);
        tags += tag.genericLabel();
    }
    */

    return tags;
}

QString KBalooRolesProvider::orientationFromValue(int value) const
{
    QString string;
    switch (value) {
    case 1: string = i18nc("@item:intable Image orientation", "Unchanged"); break;
    case 2: string = i18nc("@item:intable Image orientation", "Horizontally flipped"); break;
    case 3: string = i18nc("@item:intable image orientation", "180° rotated"); break;
    case 4: string = i18nc("@item:intable image orientation", "Vertically flipped"); break;
    case 5: string = i18nc("@item:intable image orientation", "Transposed"); break;
    case 6: string = i18nc("@item:intable image orientation", "90° rotated"); break;
    case 7: string = i18nc("@item:intable image orientation", "Transversed"); break;
    case 8: string = i18nc("@item:intable image orientation", "270° rotated"); break;
    default:
        break;
    }
    return string;
}

QString KBalooRolesProvider::durationFromValue(int value) const
{
    QTime duration;
    duration = duration.addSecs(value);
    return duration.toString("hh:mm:ss");
}

