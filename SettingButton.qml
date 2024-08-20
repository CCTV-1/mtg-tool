import QtQuick 2.12
import QtQuick.Controls 2.12

Button
{
    ToolTip
    {
        x:parent.width/2
        y:0
        visible: hovered
        text: qsTr("Setting")
        delay : 1000
    }
    display: AbstractButton.IconOnly
    highlighted: false
    flat: true
    icon.source: "qrc:/setting.svg"
}
