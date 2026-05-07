import QtQuick 2.15

import common 1.0

import components.pages 1.0
import components.texts 1.0 as Texts

import BackupCandidatesProxyModel 1.0

Item {
    id: root

    property alias footerButtons: footerButtonsItem
    readonly property int selectTableHeight: 260

    implicitHeight: layoutItem.height

    Column {
        id: layoutItem

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }

        spacing: Constants.defaultComponentSpacing

        Texts.SecondaryText {
            id: descriptionItem

            width: parent.width
            font.pixelSize: Texts.Text.Size.MEDIUM
            wrapMode: Text.WordWrap
            text: BackupsStrings.selectBackupFoldersDescription
        }

        SelectTable {
            id: backupsTable

            width: parent.width
            height: root.selectTableHeight
        }

        Item { // trick: wrapper to avoid the anchoring colision (inside the footerbuttons) with the layout manager. that's the only purpose.
            height: footerButtonsItem.height
            width: parent.width

            FooterButtons {
                id: footerButtonsItem

                leftPrimary.visible: false
                rightSecondary.text: Strings.cancel
                rightPrimary {
                    text: Strings.next
                    icons.source: Images.arrowRight
                    enabled: backupCandidatesAccess.checkAllState !== Qt.Unchecked
                }
            }
        }

        Item {
            height: Constants.defaultComponentSpacing
            width: parent.width
        }
    }
}
