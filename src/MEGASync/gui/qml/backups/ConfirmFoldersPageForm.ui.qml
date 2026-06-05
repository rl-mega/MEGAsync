import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

import common 1.0

import components.pages 1.0

import BackupCandidatesProxyModel 1.0
import BackupCandidates 1.0

Item {
    id: root

    property alias footerButtons: footerButtonsItem
    readonly property int folderContentDesignMinimumSize: 266
    readonly property int nonErrorStateFolderConfirmSpacing: 77

    implicitHeight: layoutItem.height + Constants.defaultComponentSpacing

    Column {
        id: layoutItem

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }

        ConfirmFoldersContent {
            id: contentItem

            width: parent.width
            height: Math.max(folderContentDesignMinimumSize, contentItem.implicitHeight)
        }

        Item {
            height: contentItem.errorBanner.visible ? Constants.defaultComponentSpacing : nonErrorStateFolderConfirmSpacing
            width: parent.width
        }

        Item { // trick: wrapper to avoid the anchoring colision (inside the footerbuttons) with the layout manager. that's the only purpose.
            height: footerButtonsItem.height
            width: parent.width

            FooterButtons {
                id: footerButtonsItem

                leftPrimary.visible: false
                leftSecondary {
                    text: Strings.setExclusions
                    visible: true
                    enabled: backupCandidatesAccess.globalError === BackupCandidates.NONE
                                || backupCandidatesAccess.globalError === BackupCandidates.SDK_CREATION
                }
                rightTertiary.visible: true
                rightPrimary {
                    text: BackupsStrings.backUp
                    icons.source: Images.database
                    enabled: backupCandidatesAccess.globalError === BackupCandidates.NONE
                                || backupCandidatesAccess.globalError === BackupCandidates.SDK_CREATION
                }
            }
        }

        Item {
            height: Constants.defaultComponentSpacing
            width: parent.width
        }
    }
}
