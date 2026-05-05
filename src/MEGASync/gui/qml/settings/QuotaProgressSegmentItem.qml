import QtQuick 2.15

import common 1.0
import components.texts 1.0 as Texts
import AccountStateQuickWidget 1.0

Rectangle {
    id: root

    property var segment: null
    property var childSegment: null
    property var childSegmentFillColor
    property string childTooltipText: ""
    property int minVisibleChildSegmentWidth: 2
    property bool roundLeftEdge: false
    property bool roundRightEdge: false
    property int segmentRadius: 4
    property string tooltipText: ""
    property color sizeTextColor: ColorTheme.textPrimary

    readonly property bool isFreeSegment: Number(root.segment && root.segment.type) === AccountStateQuickWidget.Free
    readonly property string centeredSizeText: root.segment && root.segment.sizeText
                                              ? root.segment.sizeText
                                              : ""
    readonly property bool showCenteredSizeText: root.isFreeSegment
                                                 && root.centeredSizeText.length > 0
                                                 && root.width >= centeredSizeTextLabel.implicitWidth + 16

    radius: (roundLeftEdge || roundRightEdge) ? segmentRadius : 0
    clip: true

    function childSegmentWidth() {
        const parentValue = Number(root.segment && root.segment.value)
        const childValue = Number(root.childSegment && root.childSegment.value)
        if (parentValue <= 0 || childValue <= 0) {
            return 0
        }

        const proportionalWidth = root.width * childValue / parentValue
        const minimumWidth = Math.max(0, root.minVisibleChildSegmentWidth)
        return Math.min(root.width,
                        Math.max(minimumWidth, proportionalWidth))
    }

    // TODO: Remove these corner-hiding helper rectangles when we move to Qt 6,
    // which supports setting the radius per corner directly.
    Rectangle {
        id: hidesRightProgressBarRoundedCorners

        anchors {
            top: parent.top
            right: parent.right
            bottom: parent.bottom
        }
        width: parent.roundLeftEdge && !parent.roundRightEdge
               ? Math.max(0, parent.width - parent.segmentRadius)
               : 0
        visible: parent.roundLeftEdge
                 && !parent.roundRightEdge
                 && parent.width > parent.segmentRadius
        color: parent.color
    }

    Rectangle {
        id: hidesLeftProgressBarRoundedCorners

        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        width: parent.roundRightEdge && !parent.roundLeftEdge
               ? Math.max(0, parent.width - parent.segmentRadius)
               : 0
        visible: parent.roundRightEdge
                 && !parent.roundLeftEdge
                 && parent.width > parent.segmentRadius
        color: parent.color
    }

    Texts.Text {
        id: centeredSizeTextLabel

        anchors.centerIn: parent
        visible: root.showCenteredSizeText
        text: root.centeredSizeText
        color: root.sizeTextColor
        font.pixelSize: 10
        font.weight: Font.DemiBold
        lineHeight: 16
        lineHeightMode: Text.FixedHeight
        z: 1
    }

    Item {
        id: childOverlayContainer

        anchors {
            top: parent.top
            right: parent.right
            bottom: parent.bottom
        }
        z: 1
        width: root.childSegmentWidth()
        visible: width > 0
        clip: true

        Rectangle {
            id: childOverlayFill

            anchors.fill: parent
            color: root.childSegmentFillColor && root.childSegment
                   ? root.childSegmentFillColor(root.childSegment)
                   : "transparent"
        }

        MouseArea {
            id: childMouseArea

            anchors.fill: parent
            hoverEnabled: true
            enabled: root.childTooltipText.length > 0

            QuotaProgressToolTip {
                id: childSegmentToolTip

                visible: parent.containsMouse && parent.enabled
                text: root.childTooltipText
                anchorItem: childOverlayContainer
            }
        }
    }

    MouseArea {
        id: parentMouseArea

        anchors.fill: parent
        hoverEnabled: true

        QuotaProgressToolTip {
            id: parentSegmentToolTip

            visible: parent.containsMouse
                     && (!childOverlayContainer.visible || !childMouseArea.containsMouse)
            text: root.tooltipText
            anchorItem: root
        }
    }
}
