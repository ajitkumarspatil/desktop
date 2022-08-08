import QtQml 2.15
import QtQuick 2.15
import QtQuick.Controls 2.3
import Style 1.0
import com.nextcloud.desktopclient 1.0

MouseArea {
    id: unifiedSearchResultMouseArea

    property string currentFetchMoreInProgressProviderId: ""

    readonly property bool isFetchMoreTrigger: model.typeAsString === "FetchMoreTrigger"

    property bool isFetchMoreInProgress: currentFetchMoreInProgressProviderId === model.providerId
    property bool isSearchInProgress: false

    property bool isPooled: false

    property var fetchMoreTriggerClicked: function(){}
    property var resultClicked: function(){}

    enabled: !isFetchMoreTrigger || !isSearchInProgress
    hoverEnabled: enabled

    height: Style.unifiedSearchItemHeight

    ToolTip {
        id: unifiedSearchResultMouseAreaTooltip
        visible: unifiedSearchResultMouseArea.containsMouse
        text: isFetchMoreTrigger ? qsTr("Load more results") : model.resultTitle + "\n\n" + model.subline
        delay: Qt.styleHints.mousePressAndHoldInterval
        contentItem: Label {
            text: unifiedSearchResultMouseAreaTooltip.text
            color: Style.ncTextColor
        }
        background: Rectangle {
            border.color: Style.menuBorder
            color: Style.backgroundColor
        }
    }

    Rectangle {
        id: unifiedSearchResultHoverBackground
        anchors.fill: parent
        color: (parent.containsMouse ? Style.lightHover : "transparent")
    }

    Loader {
        anchors.fill: parent
        active: !isFetchMoreTrigger
        sourceComponent: UnifiedSearchResultItem {
            anchors.fill: parent
            title: model.resultTitle
            subline: model.subline
            icons: Theme.darkMode ? model.darkIcons : model.lightIcons
            iconsIsThumbnail: Theme.darkMode ? model.darkIconsIsThumbnail : model.lightIconsIsThumbnail
            iconPlaceholder: Theme.darkMode ? model.darkImagePlaceholder : model.lightImagePlaceholder
            isRounded: model.isRounded && iconsIsThumbnail
        }
    }

    Loader {
        anchors.fill: parent
        active: isFetchMoreTrigger
        sourceComponent: UnifiedSearchResultFetchMoreTrigger {
            anchors.fill: parent
            isFetchMoreInProgress: unifiedSearchResultMouseArea.isFetchMoreInProgress
            isWithinViewPort: !unifiedSearchResultMouseArea.isPooled
        }
    }

    onClicked: {
        if (isFetchMoreTrigger) {
            unifiedSearchResultMouseArea.fetchMoreTriggerClicked(model.providerId)
        } else {
            unifiedSearchResultMouseArea.resultClicked(model.providerId, model.resourceUrlRole)
        }
    }
}
