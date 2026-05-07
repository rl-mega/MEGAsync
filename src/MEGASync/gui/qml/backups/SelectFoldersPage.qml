import common 1.0

SelectFoldersPageForm {
    id: root

    // added to avoid qml warning.
    function setInitialFocusPosition() { }

    signal selectFolderMoveToConfirm

    footerButtons {
        rightSecondary.onClicked: {
            window.close();
        }

        rightPrimary.onClicked: {
            root.selectFolderMoveToConfirm();
        }
    }
}
