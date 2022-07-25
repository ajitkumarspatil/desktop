/*
 * Copyright (C) 2022 by Claudio Cambra <claudio.cambra@nextcloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

import com.nextcloud.desktopclient 1.0
import Style 1.0

GridLayout {
    id: root

    signal deleteShare
    signal createNewLinkShare

    anchors.left: parent.left
    anchors.right: parent.right

    columns: 3
    rows: linkDetailLabel.visible ? 1 : 2

    columnSpacing: Style.standardSpacing / 2
    rowSpacing: Style.standardSpacing / 2

    property int iconSize: 32

    property var share: model.share
    property string iconUrl: model.iconUrl
    property string avatarUrl: model.avatarUrl
    property string text: model.display
    property string detailText: model.detailText
    property string link: model.link
    property bool isLinkShare: link.length > 0

    Item {
        id: imageItem

        property bool isAvatar: root.avatarUrl !== ""

        Layout.row: 0
        Layout.column: 0
        Layout.rowSpan: root.rows
        Layout.preferredWidth: root.iconSize
        Layout.preferredHeight: root.iconSize

        Rectangle {
            id: backgroundOrMask
            anchors.fill: parent
            radius: width / 2
            color: Style.ncBlue
            visible: !imageItem.isAvatar
        }

        Image {
            id: shareIconOrThumbnail

            anchors.centerIn: parent

            verticalAlignment: Image.AlignVCenter
            horizontalAlignment: Image.AlignHCenter
            fillMode: Image.PreserveAspectFit

            source: imageItem.isAvatar ? root.avatarUrl : root.iconUrl + "/white"
            sourceSize.width: imageItem.isAvatar ? root.iconSize : root.iconSize / 2
            sourceSize.height: imageItem.isAvatar ? root.iconSize : root.iconSize / 2

            visible: !imageItem.isAvatar
        }

        OpacityMask {
            anchors.fill: parent
            source: shareIconOrThumbnail
            maskSource: backgroundOrMask
            visible: imageItem.isAvatar
        }
    }

    Label {
        id: shareTypeLabel

        Layout.fillWidth: true
        Layout.alignment: linkDetailLabel.visible ? Qt.AlignBottom : Qt.AlignVCenter
        Layout.row: 0
        Layout.column: 1
        Layout.rowSpan: root.rows

        text: root.text
        color: Style.ncTextColor
        elide: Text.ElideRight
    }

    Label {
        id: linkDetailLabel

        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        Layout.row: 1
        Layout.column: 1

        text: root.detailText
        color: Style.ncSecondaryTextColor
        elide: Text.ElideRight
        visible: text !== ""
    }

    RowLayout {
        Layout.row: 0
        Layout.column: 2
        Layout.rowSpan: root.rows
        Layout.fillHeight: true
        spacing: 0

        Button {
            id: copyLinkButton

            Layout.alignment: Qt.AlignCenter
            Layout.preferredWidth: icon.width + (Style.standardSpacing * 2)

            flat: true
            display: AbstractButton.IconOnly
            icon.color: Style.ncTextColor
            icon.source: "qrc:///client/theme/copy.svg"
            icon.width: 16
            icon.height: 16

            enabled: root.isLinkShare
            visible: root.isLinkShare
            onClicked: {
                clipboardHelper.text = root.link;
                clipboardHelper.selectAll();
                clipboardHelper.copy();
                clipboardHelper.clear();
            }

            TextEdit { id: clipboardHelper; visible: false}
        }

        Button {
            id: moreButton

            Layout.alignment: Qt.AlignCenter
            Layout.preferredWidth: icon.width + (Style.standardSpacing * 2)

            flat: true
            display: AbstractButton.IconOnly
            icon.color: Style.ncTextColor
            icon.source: "qrc:///client/theme/more.svg"
            icon.width: 16
            icon.height: 16

            onClicked: moreMenu.popup()

            Menu {
                id: moreMenu

                MenuItem {
                    checkable: true
                    text: qsTr("Allow editing")
                }

                MenuItem {
                    checkable: true
                    text: qsTr("Hide download")
                }

                MenuItem {
                    checkable: true
                    text: qsTr("Password protect")
                }

                MenuItem {
                    checkable: true
                    text: qsTr("Set expiration date")
                }

                MenuItem {
                    checkable: true
                    text: qsTr("Note to recipient")
                }

                MenuItem {
                    icon.color: Style.ncTextColor
                    icon.source: "qrc:///client/theme/close.svg"
                    text: qsTr("Unshare")
                    onClicked: root.deleteShare()
                }

                MenuItem {
                    height: visible ? implicitHeight : 0
                    icon.color: Style.ncTextColor
                    icon.source: "qrc:///client/theme/add.svg"
                    text: qsTr("Add another link")
                    onClicked: root.createNewLinkShare()
                    visible: root.isLinkShare
                }
            }
        }
    }
}
