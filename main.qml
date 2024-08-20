import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Dialogs 1.3 as Dialogs
import QtQuick.Window 2.12
import com.mtgtool.utility 1.0

ApplicationWindow
{
    id: window
    visible: true
    width: 940
    height: 720
    //@disable-check M16
    title: qsTr("MTG Tools")

    FontLoader { id: customfont; source: "qrc:/UbuntuMono-Bold.ttf" }

    header: ToolBar
    {
        id: toolBar
        contentHeight: toolButton.implicitHeight

        SettingButton
        {
            id: toolButton
            font.pixelSize: Qt.application.font.pixelSize * 1.6
            onClicked:
            {
                var dialog_component = Qt.createComponent( "qrc:/SettingDialog.qml" )
                if ( dialog_component.status !== Component.Ready )
                {
                    if ( dialog_component.status === Component.Error )
                        console.debug( "Error:" + dialog_component.errorString() );
                    return ;
                }

                var preview_dialog = dialog_component.createObject( window , {
                    dialogWidth:window.width,
                    dialogHeight:window.height
                })

                preview_dialog.open()
            }
        }

        TextField
        {
            id:search_field
            width: parent.width - toolButton.width - 8
            anchors.right: parent.right
            placeholderText: qsTr("enter filter role string(part or all,only support english,case sensitive)")
            font.family: customfont.name
            font.pixelSize: 20
            anchors.rightMargin: 8
            onTextChanged:
            {
                sets_model.setFilterFixedString( text )
            }
        }
    }

    footer: ToolBar
    {
        Row
        {
            id:statu_layout
        }
    }

    ListView
    {
        anchors.fill: parent
        model: sets_model
        delegate:
        Rectangle
        {
            width: childrenRect.width
            height: childrenRect.height
            color: "#FFFFFF"
            border.width: 1
            border.color: "#000000"
            MouseArea
            {
                anchors.fill: parent
                hoverEnabled: true
                onEntered:
                {
                    parent.color = "#DDEEFF"
                }
                onExited:
                {
                    parent.color = "#FFFFFF"
                }
            }

            Row
            {
                id: set_row
                width: window.width
                rightPadding: 4
                leftPadding: 4
                DownloadButton
                {
                    onClicked:
                    {
                        var card_component = Qt.createComponent( "qrc:/StatusCard.qml" )
                        if ( card_component.status !== Component.Ready )
                        {
                            if ( card_component.status === Component.Error )
                                console.debug( "Error:" + card_component.errorString() );
                            return ;
                        }
                        var status_card = card_component.createObject(statu_layout, {
                            setCode:code,
                            height:statu_layout.parent.height
                        })
                        if (status_card === null)
                        {
                            console.log("Error: creating StatusCard object");
                        }
                    }
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text
                {
                    text: name
                    horizontalAlignment: Text.AlignHCenter
                    font.family: customfont.name
                    width: 400
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text
                {
                    text: code + "(" + type + ")"
                    horizontalAlignment: Text.AlignHCenter
                    font.family: customfont.name
                    width: 160
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text
                {
                    id: card_count
                    text: count
                    horizontalAlignment: Text.AlignHCenter
                    font.family: customfont.name
                    width: 160
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    onClosing:
    {
        close.accepted = false
        if ( statu_layout.children.length !== 0 )
        {
            var confirm_component = Qt.createComponent( "qrc:/ConfirmDialog.qml" )
            if ( confirm_component.status !== Component.Ready )
            {
                if ( confirm_component.status === Component.Error )
                    console.debug( "Error:" + confirm_component.errorString() )
                return
            }
            var confirm_dialog = confirm_component.createObject(window,{visible:true})
            if (confirm_dialog === null)
            {
                console.log("Error: creating StatusCard object")
            }
        }
        else
        {
            Qt.quit()
        }
    }
}
