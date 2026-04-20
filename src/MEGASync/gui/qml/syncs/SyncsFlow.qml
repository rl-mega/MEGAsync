import QtQuick 2.15

import common 1.0

import components.views 1.0

Item {
    id: root

    required property Component selectiveSyncPageComponent

    readonly property string selectiveSync: "selectiveSync"

    signal syncsFlowMoveToFinal(int syncType)
    signal syncsFlowMoveToBack()

    // added to avoid qml warning.
    function setInitialFocusPosition() { }

    implicitHeight: view.currentItem.implicitHeight
    height: implicitHeight

    state: root.selectiveSync

    states: [
        State {
            name: root.selectiveSync
            StateChangeScript {
                script: {
                    view.replace(selectiveSyncPageComponent);
                }
            }
        }
    ]

    StackViewBase {
        id: view

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }

        height: implicitHeight
        implicitHeight: currentItem.height

        onCurrentItemChanged: {
            currentItem.setInitialFocusPosition();
        }
    }
}
