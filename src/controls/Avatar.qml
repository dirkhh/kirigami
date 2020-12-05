/*
 *  SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.13
import org.kde.kirigami 2.13 as Kirigami
import QtQuick.Templates 2.13 as T
import org.kde.kirigami.private 2.13
import QtGraphicalEffects 1.0

import "templates/private" as P

/**
 * An element that represents a user, either with initials, an icon, or a profile image.
 */
T.RoundButton {
    id: root

    enum ImageMode {
        AlwaysShowImage,
        AdaptiveImageOrInitals,
        AlwaysShowInitials
    }
    enum InitialsMode {
        UseInitials,
        UseIcon
    }

    /**
    * The given name of a user.
    *
    * The user's name will be used for generating initials.
    */
    property string name

    /**
    * The source of the user's profile picture; an image.
    */
    property alias source: image.source

    /**
    * How the button should represent the user when there is no image available.
    * * `UseInitials` - Use initials when the image is not available
    * * `UseIcon` - Use an icon of a user when the image is not available
    */
    property int initialsMode: Kirigami.Avatar.InitialsMode.UseInitials

    /**
    * Whether the button should always show the image; show the image if one is
    * available and show initials when it is not; or always show initials.
    * * `AlwaysShowImage` - Always show the image; even if is not value
    * * `AdaptiveImageOrInitals` - Show the image if it is valid; or show initials if it is not
    * * `AlwaysShowInitials` - Always show initials
    */
    property int imageMode: Kirigami.Avatar.ImageMode.AdaptiveImageOrInitals

    /**
     * Whether or not the image loaded from the provided source should be cached.
     */
    property alias cache: image.cache

    /**
    * The source size of the user's profile picture.
    */
    property alias sourceSize: image.sourceSize

    /**
    * Whether or not the image loaded from the provided source should be smoothed.
    */
    property alias smooth: image.smooth

    /**
     * color: color
     *
     * The color to use for this avatar.
     */
    property var color: undefined
    // We use a var instead of a color here to allow setting the colour
    // as undefined, which will result in a generated colour being used.

    property P.BorderPropertiesGroup border: P.BorderPropertiesGroup {
        width: 0
        color: Kirigami.ColorUtils.tintWithAlpha(
            Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.2
        )
    }

    radius: height/2
    text: AvatarPrivate.initialsFromString(name).toLocaleUpperCase()
    flat: __private.showImage

    padding: root.border.width

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    palette: Kirigami.Theme.palette

    font.pixelSize: root.availableHeight/1.5 * bigFontMetrics.pixelSizeBoundingRectRatio
    font.weight: Font.Medium

    FontMetrics {
        id: bigFontMetrics
        property real pixelSizeBoundingRectRatio: bigFontMetrics.font.pixelSize/bigFontMetrics.height
        //Big enough that the way the text size is rounded shouldn't matter
        font.pointSize: 96
    }

    QtObject {
        id: __private
        // This property allows us to fall back to colour generation if
        // the root colour property is undefined.
        property color backgroundColor: {
            if (!!root.color) {
                return root.color
            }
            return AvatarPrivate.colorsFromString(name)
        }

        property color textColor: Kirigami.ColorUtils.brightnessForColor(__private.backgroundColor) == Kirigami.ColorUtils.Light
                                ? "black"
                                : "white"
        property bool showImage: {
            return (root.imageMode == Kirigami.Avatar.ImageMode.AlwaysShowImage) ||
                   (image.status == Image.Ready && root.imageMode == Kirigami.Avatar.ImageMode.AdaptiveImageOrInitals)
        }
    }

    contentItem: Item {
        implicitHeight: Kirigami.Units.iconSizes.large
        implicitWidth: Kirigami.Units.iconSizes.large
        T.Label {
            id: label
            font: root.font
            visible: root.initialsMode == Kirigami.Avatar.InitialsMode.UseInitials &&
                    !__private.showImage &&
                    !AvatarPrivate.stringUnsuitableForInitials(root.name)

            text: root.text
            color: __private.textColor

            anchors.centerIn: parent
            // Pixel aligning the coordinates isn't really needed with text,
            // so disabling it can help align things a bit better sometimes.
            anchors.alignWhenCentered: false
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            // Allows the avatar to have its size and position animated without producing distorted text
            renderType: Text.QtRendering
        }
        Kirigami.Icon {
            id: fallbackIcon
            visible: (root.initialsMode == Kirigami.Avatar.InitialsMode.UseIcon && !__private.showImage) ||
                    (AvatarPrivate.stringUnsuitableForInitials(root.name) && !__private.showImage)

            source: "user"

            anchors.centerIn: parent
            height: Kirigami.Units.fontMetrics.roundedIconSize(label.contentHeight)
            width: height

            color: __private.textColor
        }
        Image {
            id: image
            visible: __private.showImage
            asynchronous: true
            // Not using mipmap because it makes images blurry
            smooth: true
            sourceSize {
                width: image.width
                height: image.height
            }

            fillMode: Image.PreserveAspectCrop
            anchors.fill: parent
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: Rectangle {
                    anchors.centerIn: parent
                    width: root.background.width
                    height: root.background.height
                    radius: root.radius
                }
            }
        }
    }

    background: Kirigami.ShadowedRectangle {
        radius: root.radius
        color: root.flat && !__private.showImage ? __private.backgroundColor : "transparent"
        border {
            width: root.border.width
            color: root.border.color
        }
        shadow {
            color: Qt.rgba(0, 0, 0, 0.2)
            size: root.flat ? 0 : 4
        }
        Rectangle {
            visible: !__private.showImage && !root.flat
            anchors.fill: parent
            radius: parent.radius
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Qt.lighter(__private.backgroundColor, 1.1)
                }
                GradientStop {
                    position: 1.0
                    color: Qt.darker(__private.backgroundColor, 1.1)
                }
            }
        }
    }
}
