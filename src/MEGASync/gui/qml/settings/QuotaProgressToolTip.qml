import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

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
    // Updated in onVisibleChanged — mapToItem() bindings don't track ancestor position changes.
    property point parentWindowPos: Qt.point(0, 0)
    property real windowWidth: 0
    readonly property real minX: edgeMargin - parentWindowPos.x
    readonly property real maxX: windowWidth > 0
                                 ? Math.max(minX, windowWidth - width - edgeMargin - parentWindowPos.x)
                                 : minX
    readonly property real minY: edgeMargin - parentWindowPos.y
    readonly property real arrowCenterX: Math.max(radius + arrowWidth / 2 - arrowOverlap,
                                                  Math.min(width - radius - arrowWidth / 2 + arrowOverlap,
                                                           anchorCenterX - x))

    implicitWidth: bubble.implicitWidth
    implicitHeight: bubble.implicitHeight + arrowHeight - arrowOverlap
    width: implicitWidth
    height: implicitHeight
    x: anchorItem
       ? Math.round(Math.max(minX, Math.min(maxX, anchorCenterX - width / 2)))
       : 0
    y: anchorItem
       ? Math.round(Math.max(minY, anchorPosition.y - height - verticalOffset))
       : 0
    padding: 0
    closePolicy: Popup.NoAutoClose

    onVisibleChanged: {
        if (!visible || !parent) return
        // Refresh here — mapToItem() is not reactive to ancestor position changes,
        // so a binding would capture the stale pre-layout position for every segment.
        parentWindowPos = parent.mapToItem(null, 0, 0)
        windowWidth = (parent.Window.window ? parent.Window.window.width : 0)
    }

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
