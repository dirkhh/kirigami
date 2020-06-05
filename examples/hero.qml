/*
 *  SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.12
import QtQuick.Layouts 1.4
import org.kde.kirigami 2.14 as Kirigami

Kirigami.ApplicationWindow {
    id: root

    pageStack.initialPage: mainPageComponent

    Component {
        id: mainPageComponent
        Kirigami.ScrollablePage {
            title: "Hello"
            RowLayout {
                Kirigami.Avatar {
                    id: avatar
                    name: "John Doe"
                    TapHandler {
                        onTapped: {
                            var page = root.pageStack.push(secondPageComponent)
                            page.hero.source = avatar
                            page.hero.open();
                        }
                    }
                }
            }
        }
    }

    Component {
        id: secondPageComponent
        Kirigami.ScrollablePage {
            id: page
            title: "Second Page"
            property Kirigami.Hero hero: Kirigami.Hero {
                destination: headerRect
            }
            ColumnLayout {
                Rectangle {
                    id: headerRect
                    color: "green"
                    Layout.fillWidth: true
                    implicitHeight: Kirigami.Units.gridUnit * 20
                    TapHandler {
                        onTapped: {
                            page.hero.close();
                        }
                    }
                }
                Item {
                    Layout.fillWidth: true
                    implicitHeight: Kirigami.Units.gridUnit * 40
                }
            }
        }
    }
}
