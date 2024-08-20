import QtQuick 2.0
import QtQuick.Window 2.2
import QtQuick.Controls 2.5
import QtQuick.Dialogs 1.3 as Dialogs

Dialog {
    id: setting_dialog

    property int dialogWidth: 940
    property int dialogHeight: 720

    width: dialogWidth
    height: dialogHeight
    visible: true

    Column
    {
        id: column
        width: parent.width
        spacing: 10

        Row
        {
            width: parent.width
            Text
            {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width*2/5
                text: qsTr("filter role:")
                horizontalAlignment: Text.AlignHCenter
                font.family: customfont.name
                font.pixelSize: 14
            }
            ComboBox
            {
                anchors.verticalCenter: parent.verticalCenter
                model: role_model.enums
                width: parent.width*3/5
                currentIndex: role_model.enumvalue_to_index( tool_settings.get_filter_role() )
                font.family: customfont.name
                onActivated:
                {
                    var enumvalue = role_model.index_to_enumvalue( currentIndex );
                    sets_model.set_fileter_role( enumvalue )
                    tool_settings.set_filter_role( enumvalue )
                }

                delegate: ItemDelegate
                {
                    text: modelData
                    width:parent.width
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.family: customfont.name
                }
            }
        }

        Row
        {
            width: parent.width
            Text
            {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width*2/5
                text: qsTr("image language:")
                horizontalAlignment: Text.AlignHCenter
                font.family: customfont.name
                font.pixelSize: 14
            }
            ComboBox
            {
                anchors.verticalCenter: parent.verticalCenter
                model: lang_model.enums
                width: parent.width*3/5
                currentIndex:
                lang_model.enumvalue_to_index( tool_settings.get_image_lanuage() )
                font.family: customfont.name
                onActivated:
                {
                    var enum_value = lang_model.index_to_enumvalue( currentIndex )
                    tool_settings.set_image_lanuage( enum_value )
                }

                delegate: ItemDelegate
                {
                    text: modelData
                    width:parent.width
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.family: customfont.name
                }
            }
        }

        Row
        {
            width: parent.width
            Text
            {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width*2/5
                text: qsTr("image styles:")
                horizontalAlignment: Text.AlignHCenter
                font.family: customfont.name
                font.pixelSize: 14
            }
            ComboBox
            {
                anchors.verticalCenter: parent.verticalCenter
                model: image_styles_model.enums
                width: parent.width*3/5
                currentIndex:
                image_styles_model.enumvalue_to_index( tool_settings.get_image_resolution() )
                font.family: customfont.name
                onActivated:
                {
                    var enum_value = image_styles_model.index_to_enumvalue( currentIndex )
                    tool_settings.set_image_resolution( enum_value )
                }

                delegate: ItemDelegate
                {
                    text: modelData
                    width: parent.width
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.family: customfont.name
                }
            }
        }

        Row
        {
            width: parent.width
            Text
            {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width*2/5
                text: qsTr("image name format:")
                horizontalAlignment: Text.AlignHCenter
                font.family: customfont.name
                font.pixelSize: 14
            }
            ComboBox
            {
                anchors.verticalCenter: parent.verticalCenter
                model: nameformat_model.enums
                width: parent.width*3/5
                currentIndex:
                nameformat_model.enumvalue_to_index( tool_settings.get_image_name_format() )
                font.family: customfont.name
                onActivated:
                {
                    var enum_value = nameformat_model.index_to_enumvalue( currentIndex )
                    tool_settings.set_image_name_format( enum_value )
                }

                delegate: ItemDelegate
                {
                    text: modelData
                    width: parent.width
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.family: customfont.name
                }
            }
        }
        Row
        {
            width: parent.width
            Text
            {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width*2/5
                text: qsTr("image cache directory:")
                horizontalAlignment: Text.AlignHCenter
                font.family: customfont.name
                font.pixelSize: 14
            }
            Button
            {
                id: select_dir_button
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width*3/5
                text: tool_settings.get_cache_directory()
                font.family: customfont.name
                Dialogs.FileDialog
                {
                    id: download_dir_chooser
                    title: qsTr( "choose download image cache directory" )
                    selectFolder: true
                    selectMultiple: false
                    onAccepted:
                    {
                        select_dir_button.text = download_dir_chooser.fileUrl
                        tool_settings.set_cache_directory( download_dir_chooser.fileUrl )
                    }
                }
                onClicked:
                {
                    download_dir_chooser.open()
                }
            }
        }
        Row
        {
            width: parent.width
            Text
            {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width*2/5
                text: qsTr("set data timeout delay:")
                horizontalAlignment: Text.AlignHCenter
                font.family: customfont.name
                font.pixelSize: 14
            }
            SpinBox
            {
                id: set_data_timeout_delay
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width*3/5
                editable: true
                from: 1
                to: Screen.width
                value: tool_settings.get_data_update_delay()
                font.family: customfont.name
                onValueModified:
                {
                    tool_settings.set_data_update_delay( value )
                }
            }
        }
        Row
        {
            width: parent.width
            Text
            {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width*2/5
                text: qsTr("preivew image width:")
                horizontalAlignment: Text.AlignHCenter
                font.family: customfont.name
                font.pixelSize: 14
            }
            SpinBox
            {
                id: image_width_box
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width*3/5
                editable: true
                from: 1
                to: Screen.width
                value: tool_settings.get_image_width()
                font.family: customfont.name
                onValueModified:
                {
                    tool_settings.set_image_width( value )
                }
            }
        }
        Row
        {
            width: parent.width
            Text
            {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width*2/5
                text: qsTr("preivew image height:")
                horizontalAlignment: Text.AlignHCenter
                font.family: customfont.name
                font.pixelSize: 14
            }
            SpinBox
            {
                id: image_height_box
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width*3/5
                editable: true
                from: 1
                to: Screen.height
                value: tool_settings.get_image_height()
                font.family: customfont.name
                onValueModified:
                {
                    tool_settings.set_image_height( value )
                }
            }
        }
        Button
        {
            text: qsTr( "close" )
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked:setting_dialog.destroy()
        }
    }
}
