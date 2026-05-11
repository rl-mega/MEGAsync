import QtQuick 2.15

import common 1.0

import components.views 1.0
import components.steps 1.0

import BackupsComponent 1.0
import SyncInfo 1.0

import ServiceUrls 1.0

SyncsQmlDialog {
    id: window

    title: BackupsStrings.backupsWindowTitle
    visible: false
    modality: Qt.NonModal
    minimumHeight: backupsContentItem.implicitHeight
    maximumHeight: backupsContentItem.implicitHeight
    height: minimumHeight
    width: 640
    maximumWidth: width
    minimumWidth: width
    backup: true
    closeOnEscapePressed: true
    color: ColorTheme.surface1

    readonly property int defaultWindowMargin: 24

    Behavior on height {
        // Only animate height changes once the dialog has finished its
        // initial show-and-position phase. Otherwise the multi-pass QML
        // layout would animate from the first computed implicitHeight to
        // the final one while the user can already see the dialog,
        // producing a visible "shrink" glitch.
        enabled: window.initialLayoutComplete
        NumberAnimation {
            duration: 200
            easing.type: Easing.InOutQuad
        }
    }

    Rectangle {
        id: backupsContentItem

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }

        color: ColorTheme.surface1

        implicitHeight : stackView.implicitHeight
        height: implicitHeight

        readonly property string backupsFlow: "backupsFlow"
        readonly property string resume: "resume"

        state: backupsFlow
        states: [
            State {
                name: backupsContentItem.backupsFlow
                StateChangeScript {
                    script: stackView.replace(backupsFlowPage);
                }
            },
            State {
                name: backupsContentItem.resume
                StateChangeScript {
                    script: stackView.replace(resumePage);
                }
            }
        ]

        StackViewBase {
            id: stackView

            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                margins: defaultWindowMargin
            }

            implicitHeight: currentItem.implicitHeight

            Component {
                id: backupsFlowPage

                BackupsPage {
                    id: backupsFlowItem

                    backupsContentItemRef: backupsContentItem
                }
            }

            Component {
                id: resumePage

                ResumePage {
                    id: resumePageItem
                }
            }
        }
    }
}
