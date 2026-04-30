import QtQuick 2.15
import QtQuick.Controls 2.15

import common 1.0
import components.texts 1.0 as Texts

Popup {
    id: root

    property string text: ""
    property Item anchorItem: null

    readonly property int verticalOffset: 10
    readonly property int verticalContentMargin: 4
    readonly property int horizontalContentMargin: 8
    readonly property int radius: 4
    readonly property int arrowWidth: 12
    readonly property int arrowHeight: 6
    readonly property int arrowOverlap: 1
    readonly property int maxTextWidth: 160
    readonly property int edgeMargin: 0
    readonly property var anchorPosition: anchorItem && parent
                                        ? anchorItem.mapToItem(parent, 0, 0)
                                        : Qt.point(0, 0)
    readonly property real anchorCenterX: anchorPosition.x + (anchorItem ? anchorItem.width / 2 : 0)
    readonly property real textWidth: Math.ceil(Math.min(maxTextWidth,
                                                         Math.max(0, sizingText.paintedWidth)))
    readonly property real unclampedX: anchorCenterX - width / 2
    readonly property real minX: edgeMargin
    readonly property real maxX: parent ? Math.max(edgeMargin, parent.width - width - edgeMargin) : edgeMargin
    readonly property real arrowCenterX: Math.max(radius + arrowWidth / 2 - arrowOverlap,
                                                  Math.min(width - radius - arrowWidth / 2 + arrowOverlap,
                                                           anchorCenterX - x))

    implicitWidth: bubble.implicitWidth
    implicitHeight: bubble.implicitHeight + arrowHeight - arrowOverlap
    width: implicitWidth
    height: implicitHeight
    x: anchorItem
       ? Math.round(Math.max(minX, Math.min(maxX, unclampedX)))
       : 0
    y: anchorItem
       ? Math.round(anchorPosition.y - height - verticalOffset)
       : 0
    padding: 0
    modal: false
    focus: false
    closePolicy: Popup.NoAutoClose

    background: Item {
        Rectangle {
            id: bubble

            width: root.width
            height: bubble.implicitHeight
            color: ColorTheme.surfaceInverseAccent
            radius: root.radius
            implicitWidth: root.textWidth
                           + root.horizontalContentMargin * 2
            implicitHeight: bodyText.implicitHeight
                            + root.verticalContentMargin * 2

            Texts.Text {
                id: sizingText

                visible: false
                width: root.maxTextWidth
                text: root.text
                color: ColorTheme.textInverseAccent
                font.pixelSize: 10
                font.weight: Font.DemiBold
                lineHeight: 16
                lineHeightMode: Text.FixedHeight
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.Wrap
                renderType: Text.NativeRendering
            }

            Texts.Text {
                id: bodyText

                anchors {
                    fill: parent
                    leftMargin: root.horizontalContentMargin
                    topMargin: root.verticalContentMargin
                    rightMargin: root.horizontalContentMargin
                    bottomMargin: root.verticalContentMargin
                }
                width: root.textWidth
                text: root.text
                color: ColorTheme.textInverseAccent
                font.pixelSize: 10
                font.weight: Font.DemiBold
                font.styleName: "Semi Bold"
                lineHeight: 16
                lineHeightMode: Text.FixedHeight
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.Wrap
                renderType: Text.NativeRendering
            }
        }

        Canvas {
            id: arrow

            anchors.top: bubble.bottom
            anchors.topMargin: -root.arrowOverlap
            width: root.arrowWidth
            height: root.arrowHeight
            x: Math.round(root.arrowCenterX - width / 2)

            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                ctx.fillStyle = ColorTheme.surfaceInverseAccent
                ctx.beginPath()
                ctx.moveTo(0, 0)
                ctx.lineTo(width / 2, height)
                ctx.lineTo(width, 0)
                ctx.closePath()
                ctx.fill()
            }
        }
    }
}
