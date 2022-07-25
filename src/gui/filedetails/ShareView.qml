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
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.15

import com.nextcloud.desktopclient 1.0
import Style 1.0

ColumnLayout {
    id: root

    property string localPath: ""
    property var accountState: ({})
    property var fileDetails: ({})
    property int horizontalPadding: 0
    property int iconSize: 32

    property ShareModel shareModel: ShareModel {
        accountState: root.accountState
        localPath: root.localPath
    }

    ShareeSearchField {
        Layout.fillWidth: true
        Layout.leftMargin: root.horizontalPadding
        Layout.rightMargin: root.horizontalPadding

        accountState: root.accountState
        shareItemIsFolder: root.fileDetails && root.fileDetails.isFolder

        onShareeSelected: root.shareModel.createNewUserGroupShareFromQml(sharee)
    }

    ScrollView {
        id: scrollView
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: root.horizontalPadding
        Layout.rightMargin: root.horizontalPadding

        contentWidth: availableWidth
        clip: true

        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        data: WheelHandler {
            target: scrollView.contentItem
        }

        ListView {
            id: shareLinksListView
            model: root.shareModel

            delegate: ShareLinkDelegate {
                iconSize: root.iconSize
                onCreateNewLinkShare: shareModel.createNewLinkShare()
                onDeleteShare: shareModel.deleteShare(model.share)
            }
        }
    }
}
