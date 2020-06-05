/*
 *  SPDX-FileCopyrightText: 2020 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.14
import QtQuick.Controls 2.4 as QQC2
import org.kde.kirigami 2.13 as Kirigami

//FIXME: QtObject
Item {
    id: root

    property Item source:Item{}
    property Item destination:Item{}

    function open() {
        openAnim.restart();
    }
    function close() {
        closeAnim.restart();
    }

    SequentialAnimation {
        id: openAnim
        ScriptAction {
            script: {
                root.source.layer.enabled = true;
                root.source.layer.smooth = true;
                root.destination.layer.enabled = true;
                root.destination.layer.smooth = true;
                sourceEffect.visible = true;
                destinationEffect.visible = true;
                sourceEffect.source = null
                sourceEffect.source = root.source
                destinationEffect.source = null
                destinationEffect.source = root.destination
                root.source.opacity = 0
                root.destination.opacity = 0
                print("AAAAA"+root.source+root.destination+sourceEffect.parent)
                sourceEffect.parent.visible = true;
            }
        }
        ParallelAnimation {
            NumberAnimation {
                target: sourceEffect
                property: "progress"
                from: 0
                to: 1
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                target: destinationEffect
                property: "progress"
                from: 1
                to: 0
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }
        ScriptAction {
            script: {
                sourceEffect.visible = false;
                destinationEffect.visible = false;
                root.source.layer.enabled = false;
                root.source.layer.smooth = false;
                root.destination.layer.enabled = false;
                root.destination.layer.smooth = false;
                root.destination.opacity = 1
                sourceEffect.parent.visible = false;
            }
        }

    }
    SequentialAnimation {
        id: closeAnim
        ScriptAction {
            script: {
                root.source.layer.enabled = true;
                root.source.layer.smooth = true;
                root.destination.layer.enabled = true;
                root.destination.layer.smooth = true;
                sourceEffect.visible = true;
                destinationEffect.visible = true;
                sourceEffect.source = null
                sourceEffect.source = root.source
                destinationEffect.source = null
                destinationEffect.source = root.destination
                root.source.opacity = 0
                root.destination.opacity = 0
            }
        }
        ParallelAnimation {
            NumberAnimation {
                target: sourceEffect
                property: "progress"
                from: 1
                to: 0
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                target: destinationEffect
                property: "progress"
                from: 0
                to: 1
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }
        ScriptAction {
            script: {
                sourceEffect.visible = false;
                destinationEffect.visible = false;
                root.source.layer.enabled = false;
                root.source.layer.smooth = false;
                root.destination.layer.enabled = false;
                root.destination.layer.smooth = false;
                root.source.opacity = 1
            }
        }
    }


    ShaderEffect {
        id: sourceEffect
        x: 0
        y: 0
        parent: root.source.QQC2.Overlay.overlay
        width: root.source.width
        height: root.source.height
        visible: false
        property variant source: root.source
        property real progress: 0
        property real startX: root.source.Kirigami.ScenePosition.x / (applicationWindow().width / 2)
        property real startY: -root.source.Kirigami.ScenePosition.y / (applicationWindow().height / 2)

        property real targetX: scaleWidth - 1 + (root.destination.Kirigami.ScenePosition.x * 2) / applicationWindow().width
        property real targetY: 1-scaleHeight - (root.destination.Kirigami.ScenePosition.y * 2) / applicationWindow().height
        property real scaleWidth: root.destination.width/root.source.width
        property real scaleHeight: root.destination.height/root.source.height
        vertexShader: "
            uniform highp mat4 qt_Matrix;
            attribute highp vec4 qt_Vertex;
            attribute highp vec2 qt_MultiTexCoord0;
            varying highp vec2 qt_TexCoord0;
            uniform highp float startX;
            uniform highp float startY;
            uniform highp float targetX;
            uniform highp float targetY;
            uniform highp float scaleWidth;
            uniform highp float scaleHeight;
            uniform highp float progress;

            highp mat4 morph = mat4(1.0 + (scaleWidth - 1.0) * progress, 0.0, 0.0, startX*(1.0-progress) + targetX*progress,
                                    0.0, 1.0 + (scaleHeight - 1.0) * progress, 0.0, startY*(1.0-progress) + targetY*progress,
                                    0.0, 0.0, 1.0, 0.0,
                                    0.0, 0.0, 0.0, 1.0);

            void main() {
                qt_TexCoord0 = qt_MultiTexCoord0;
                gl_Position = qt_Matrix * qt_Vertex * morph;
            }"
        fragmentShader: "
            varying highp vec2 qt_TexCoord0;
            uniform sampler2D source;
            uniform lowp float qt_Opacity;
            uniform lowp float progress;
            void main() {
                gl_FragColor = texture2D(source, qt_TexCoord0) * qt_Opacity * (1.0 - progress);
            }"
    }

    //TODO: deduplicate all this code
    ShaderEffect {
        id: destinationEffect
        x: 0
        y: 0
        parent: root.destination.QQC2.Overlay.overlay
        width: root.destination.width
        height: root.destination.height
        visible: false
        property variant source: root.destination
        property real progress: sourceEffect.progress
        property real startX: root.destination.Kirigami.ScenePosition.x / (applicationWindow().width / 2)
        property real startY: -root.destination.Kirigami.ScenePosition.y / (applicationWindow().height / 2)

        property real targetX: scaleWidth - 1 + (root.source.Kirigami.ScenePosition.x * 2) / applicationWindow().width
        property real targetY: 1-scaleHeight - (root.source.Kirigami.ScenePosition.y * 2) / applicationWindow().height
        property real scaleWidth: root.source.width/root.destination.width
        property real scaleHeight: root.source.height/root.destination.height

        property variant maskSource: mask
        //TODO: this should be user-customizable
        Rectangle {
            id: mask
            visible: false
            color: "white"
            radius: (width/2) * destinationEffect.progress
            width: (sourceEffect.width * sourceEffect.progress) +  (destinationEffect.width * destinationEffect.progress)
            height: (sourceEffect.height * sourceEffect.progress) +  (destinationEffect.height * destinationEffect.progress)
            layer.enabled: true
            layer.smooth: true
        }
        vertexShader: "
            uniform highp mat4 qt_Matrix;
            attribute highp vec4 qt_Vertex;
            attribute highp vec2 qt_MultiTexCoord0;
            varying highp vec2 qt_TexCoord0;
            uniform highp float startX;
            uniform highp float startY;
            uniform highp float targetX;
            uniform highp float targetY;
            uniform highp float scaleWidth;
            uniform highp float scaleHeight;
            uniform highp float progress;

            highp mat4 morph = mat4(1.0 + (scaleWidth - 1.0) * progress, 0.0, 0.0, startX*(1.0-progress) + targetX*progress,
                                    0.0, 1.0 + (scaleHeight - 1.0) * progress, 0.0, startY*(1.0-progress) + targetY*progress,
                                    0.0, 0.0, 1.0, 0.0,
                                    0.0, 0.0, 0.0, 1.0);

            void main() {
                qt_TexCoord0 = qt_MultiTexCoord0;
                gl_Position = qt_Matrix * qt_Vertex * morph;
            }"
        fragmentShader: "
            varying highp vec2 qt_TexCoord0;
            uniform sampler2D source;
            uniform sampler2D maskSource;
            uniform lowp float qt_Opacity;
            uniform lowp float progress;
            void main() {
                gl_FragColor = texture2D(source, qt_TexCoord0) * texture2D(maskSource, qt_TexCoord0).a * qt_Opacity *  (1.0 - progress);
            }"
    }
}
