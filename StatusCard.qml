import QtQuick 2.0
import com.mtgtool.utility 1.0

Rectangle {
    id:root
    property string setCode: "test"

    radius: 4
    color: "transparent"
    border.width: 1
    border.color: "#000"
    implicitHeight: 40
    implicitWidth: 100

    Downloader
    {
        id: downloader
        onRequest_count_changed:
        {
            status.text = setCode + ":" + request_count;
        }
        onDownload_end:
        {
            root.destroy()
        }
    }

    Text {
        id: status
        color: "#ffffff"
        text: setCode
        verticalAlignment: Text.AlignVCenter
        anchors.fill: parent
        horizontalAlignment: Text.AlignHCenter
        styleColor: "#ffffff"
    }

    Component.onCompleted: downloader.download_set(setCode)
}
