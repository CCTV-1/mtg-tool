import QtQuick 2.12
import QtQuick.Controls 2.15

Button
{
    display: AbstractButton.IconOnly
    icon.source: "qrc:/download.svg"
    ToolTip
    {
        x:parent.width/2
        y:0
        visible: hovered
        text: qsTr("Download")
        delay : 1000
    }
}
