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

TextField {
    id: root

    signal shareeSelected(var sharee)

    property var accountState: ({})
    property bool shareItemIsFolder: false

    function triggerSuggestionsVisibility() {
        shareeListView.count > 0 && text !== "" ? suggestionsPopup.open() : suggestionsPopup.close();
    }

    placeholderText: qsTr("Search for users or groups…")
    placeholderTextColor: Style.menuBorder
    color: Style.ncTextColor

    onActiveFocusChanged: triggerSuggestionsVisibility()
    onTextChanged: triggerSuggestionsVisibility()

    background: Rectangle {
        radius: 5
        border.color: parent.activeFocus ? UserModel.currentUser.accentColor : Style.menuBorder
        border.width: 1
        color: Style.backgroundColor
    }

    Popup {
        id: suggestionsPopup
        width: root.width
        height: 100
        y: root.height

        contentItem: ScrollView {
            clip: true

            ListView {
                id: shareeListView

                onCountChanged: root.triggerSuggestionsVisibility()

                model: ShareeModel {
                    accountState: root.accountState
                    shareItemIsFolder: root.shareItemIsFolder
                    searchString: root.text
                }

                delegate: ShareeDelegate {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    onClicked: {
                        root.shareeSelected(model.sharee);
                        suggestionsPopup.close();
                    }
                }
            }
        }
    }
}
