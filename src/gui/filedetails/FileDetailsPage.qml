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
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

import com.nextcloud.desktopclient 1.0
import Style 1.0

Page {
    id: root

    property var accountState: ({})
    property string localPath: ({})

    // We want the SwipeView to "spill" over the edges of the window to really
    // make it look nice. If we apply page-wide padding, however, the swipe
    // contents only go as far as the page contents, clipped by the padding.
    // This property reflects the padding we intend to display, but not the real
    // padding, which we have to apply selectively to achieve our desired effect.
    property int intendedPadding: Style.standardSpacing * 2
    property int iconSize: 32

    property alias fileDetails: fileDetails

    FileDetails {
        id: fileDetails
        localPath: root.localPath
    }

    topPadding: intendedPadding
    bottomPadding: intendedPadding

    background: Rectangle {
        color: Style.backgroundColor
    }

    header: Column {
        spacing: root.intendedPadding
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right

            margins: root.intendedPadding
        }

        GridLayout {
            columns: 2
            rows: 2

            rowSpacing: Style.standardSpacing / 2
            columnSpacing: Style.standardSpacing

            Image {
                id: fileIcon

                Layout.rowSpan: 2
                Layout.preferredWidth: implicitWidth
                Layout.fillHeight: true

                verticalAlignment: Image.AlignVCenter
                horizontalAlignment: Image.AlignHCenter
                source: fileDetails.iconUrl
                sourceSize.width: Style.trayListItemIconSize
                sourceSize.height: Style.trayListItemIconSize
                fillMode: Image.PreserveAspectFit
            }

            Label {
                id: fileNameLabel

                Layout.fillWidth: true

                text: fileDetails.name
                color: Style.ncTextColor
                font.bold: true
            }

            Label {
                id: fileDetailsLabel

                Layout.fillWidth: true

                text: `${fileDetails.sizeString} · ${fileDetails.lastChangedString}`
                color: Style.ncSecondaryTextColor
            }
        }

        TabBar {
            id: viewBar

            padding: 0
            background: Rectangle {
                color: Style.backgroundColor
            }

            NCTabButton {
                svgCustomColorSource: "image://svgimage-custom-color/activity.svg"
                text: qsTr("Activity")
                checked: swipeView.currentIndex === fileActivityView.swipeIndex
                onClicked: swipeView.currentIndex = fileActivityView.swipeIndex
            }

            NCTabButton {
                svgCustomColorSource: "image://svgimage-custom-color/share.svg"
                text: qsTr("Sharing")
                checked: swipeView.currentIndex === shareView.swipeIndex
                onClicked: swipeView.currentIndex = shareView.swipeIndex
            }
        }
    }

    SwipeView {
        id: swipeView

        anchors.fill: parent
        anchors.topMargin: Style.standardSpacing
        clip: true

        FileActivityView {
            id: fileActivityView

            property int swipeIndex: SwipeView.index

            accountState: root.accountState
            localPath: root.localPath
            horizontalPadding: root.intendedPadding
            iconSize: root.iconSize
        }

        ShareView {
            id: shareView

            // These are attached properties
            property int swipeIndex: SwipeView.index

            accountState: root.accountState
            localPath: root.localPath
            fileDetails: fileDetails
            horizontalPadding: root.intendedPadding
            iconSize: root.iconSize
        }
    }
}
