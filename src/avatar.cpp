// SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>
//
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "avatar.h"
#include <QQuickStyle>
#include <QMap>
#include <QVector>
#include <QStringRef>
#include <QTextBoundaryFinder>
#include <QDebug>

bool contains(const QString& str, QChar::Script s) {
    for (auto rune : str) {
        if (rune.script() == s) {
            return true;
        }
    }
    return false;
}

QString AvatarPrivate::initialsFromString(const QString& string)
{
    // "" -> ""
    if (string.isEmpty()) return {};

    auto normalized = string.normalized(QString::NormalizationForm_D);

    // Names written with Han and Hangul characters generally can be initialised by taking the
    // first character
    if (contains(normalized, QChar::Script_Han) || contains(normalized, QChar::Script_Hangul)) {
        return QString(normalized[0]);
    }

    // "FirstName Name Name LastName"
    if (normalized.contains(QLatin1Char(' '))) {
        // "FirstName Name Name LastName" -> "FirstName" "Name" "Name" "LastName"
        const auto split = normalized.splitRef(QLatin1Char(' '));
        // "FirstName"
        auto first = split.first();
        // "LastName"
        auto last = split.last();
        if (first.isEmpty()) {
            // "" "LastName" -> "L"
            return QString(last.front());
        }
        if (last.isEmpty()) {
            // "FirstName" "" -> "F"
            return QString(first.front());
        }
        // "FirstName" "LastName" -> "FL"
        return QString(first.front()) + last.front();
    // "OneName"
    } else {
        // "OneName" -> "O"
        return QString(normalized.front());
    }
}

const QMap<QString,QList<QColor>> c_colors = {
    {
        QStringLiteral("default"),
        {
            QColor("#e93a9a"),
            QColor("#e93d58"),
            QColor("#e9643a"),
            QColor("#ef973c"),
            QColor("#e8cb2d"),
            QColor("#b6e521"),
            QColor("#3dd425"),
            QColor("#00d485"),
            QColor("#00d3b8"),
            QColor("#3daee9"),
            QColor("#b875dc"),
            QColor("#926ee4"),
        }
    },
    {
        QStringLiteral("Material"),
        {
            QColor("#f44336"),
            QColor("#e91e63"),
            QColor("#9c27b0"),
            QColor("#673ab7"),
            QColor("#3f51b5"),
            QColor("#2196f3"),
            QColor("#03a9f4"),
            QColor("#00bcd4"),
            QColor("#009688"),
            QColor("#4caf50"),
            QColor("#8bc34a"),
            QColor("#cddc39"),
            QColor("#ffeb3b"),
            QColor("#ffc107"),
            QColor("#ff9800"),
            QColor("#ff5722"),
        }
    }
};

QList<QColor> grabColors()
{
    if (c_colors.contains(QQuickStyle::name())) {
        return c_colors[QQuickStyle::name()];
    }
    return c_colors[QStringLiteral("default")];
}

auto AvatarPrivate::colorsFromString(const QString& string) -> QColor
{
    // We use a hash to get a "random" number that's always the same for
    // a given string.
    auto hash = qHash(string);
    // hash modulo the length of the colors list minus one will always get us a valid
    // index
    auto index = hash % (grabColors().length()-1);
    // return a colour
    return grabColors()[index];
}

auto AvatarPrivate::stringUnsuitableForInitials(const QString& string) -> bool
{
    if (string.isEmpty()) {
        return true;
    }

    bool isNumber;
    string.toFloat(&isNumber);
    if (isNumber) {
        return true;
    }

    const auto scripts = QList<QChar::Script> { QChar::Script_Common, QChar::Script_Inherited, QChar::Script_Latin, QChar::Script_Han, QChar::Script_Hangul };

    for (auto character : string) {
        if (!scripts.contains(character.script())) {
            return true;
        }
    }
    return false;
}
