import QtQuick 2.15

import common 1.0
import components.views 1.0

import SyncsComponents 1.0
import SyncInfo 1.0
import ServiceUrls 1.0

SyncsQmlDialog {
    id: window

    title: SyncsStrings.syncsWindowTitle
    visible: false
    modality: Qt.NonModal
    width: 640
    height: syncsContentItem.implicitHeight
    maximumHeight: height
    maximumWidth: width
    minimumHeight: height
    minimumWidth: width
    closeOnEscapePressed: true

    readonly property int defaultWindowMargin: 24

    Rectangle {
        id: syncsContentItem

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }

        color: ColorTheme.surface1

        height: implicitHeight
        implicitHeight : stackView.implicitHeight

        readonly property string syncsFlow: "syncsFlow"
        readonly property string resume: "resume"

        state: syncsFlow
        states: [
            State {
                name: syncsContentItem.syncsFlow
                StateChangeScript {
                    script: stackView.replace(syncsFlowPage);
                }
            },
            State {
                name: syncsContentItem.resume
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
            }
            height: implicitHeight
            implicitHeight: currentItem.height

            Component {
                id: syncsFlowPage

                SyncsPage {
                    id: syncsFlowItem

                    syncsContentItemRef: syncsContentItem
                }
            }

            Component {
                id: resumePage

                ResumeSyncsPage {
                    id: resumeSyncsPageItem

                    footerButtons.leftPrimary.visible: false
                }
            }
        }
    }
}
