import QtQuick 2.0
import QtQuick.Controls 2.5

Dialog
{
    id:root
    anchors.centerIn: parent
    modal: true
    title: qsTr("There are still unfinished download tasks. Are you sure you want to close?")
    standardButtons: Dialog.Ok | Dialog.Cancel
    onAccepted: Qt.quit()
    onRejected: root.destroy()
}
