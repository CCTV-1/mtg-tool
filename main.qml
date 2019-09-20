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
    title: qsTr("MTG Tools")

    FontLoader { id: customfont; source: "qrc:/UbuntuMono-Bold.ttf" }

    Downloader { id: deck_downloader }

    function open_preview_dialog( deck )
    {
        if ( deck === null )
            return
        var main_cards = deck.main_list()
        var side_cards = deck.side_list()

        var main_source_urls = []
        for ( var i in main_cards )
        {
            if ( main_cards[i].exists_localfile() )
            {
                main_source_urls.push( main_cards[i].local_uri().toString() )
            }
            else
            {
                deck_downloader.download_card( main_cards[i].local_url() , main_cards[i].scryfall_url() )
                main_source_urls.push( main_cards[i].scryfall_url().toString() )
            }

        }
        var side_source_urls = []
        for ( var j in side_cards )
        {
            if ( side_cards[j].exists_localfile() )
            {
                side_source_urls.push( side_cards[j].local_uri().toString() )
            }
            else
            {
                deck_downloader.download_card( side_cards[j].local_url() , side_cards[j].scryfall_url() )
                side_source_urls.push( side_cards[j].scryfall_url().toString() )
            }
        }

        var dialog_component = Qt.createComponent( "PreviewDialog.qml" )
        if ( dialog_component.status !== Component.Ready )
        {
            if ( dialog_component.status === Component.Error )
                console.debug( "Error:" + dialog_component.errorString() );
            return ;
        }

        var preview_dialog = dialog_component.createObject( window ,
        {
            image_width: image_width_box.value,
            image_height: image_height_box.value,
            main_image_list : main_source_urls,
            sideboard_image_list : side_source_urls
        })

        preview_dialog.open()
    }

    Page
    {
        id: setting_page
        visible: false
        title: qsTr("Download Preferences")

        Grid
        {
            anchors.fill: parent
            columns: 1
            rowSpacing: 10

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
            Row
            {
                width: parent.width
                Text
                {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width*2/5
                    text: qsTr("sets views button function:")
                    horizontalAlignment: Text.AlignHCenter
                    font.family: customfont.name
                    font.pixelSize: 14
                }
                ComboBox
                {
                    id: func_control
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width*3/5
                    currentIndex: 0
                    model:
                    [
                        qsTr( "download sets image" ),
                        qsTr( "generator sets ranking table" )
                    ]
                }
            }
        }
    }

    Page
    {
        id: deckpreview_generator
        visible: false
        title: qsTr( "preview generator" )
        DeckParser
        {
            id: parser
        }
        Grid
        {
            anchors.fill: parent
            columns: 1
            rowSpacing: 10
            Button
            {
                text: qsTr("parse clipboard")
                font.capitalization: Font.MixedCase
                width: parent.width
                font.family: customfont.name
                onClicked:
                {
                    var deck = parser.parse_clipboard()
                    open_preview_dialog( deck )
                }
            }
            Button
            {
                Dialogs.FileDialog
                {
                    id: target_file
                    title: qsTr("choose a deck file")
                    onAccepted:
                    {
                        var deck = parser.parse_file( target_file.fileUrl )
                        open_preview_dialog( deck )
                    }
                }
                text: qsTr( "parse deck" )
                font.capitalization: Font.MixedCase
                width: window.width
                font.family: customfont.name
                onClicked:
                {
                    target_file.open()
                }
            }
        }
    }

    header: ToolBar
    {
        contentHeight: toolButton.implicitHeight

        ToolButton
        {
            id: toolButton
            text: stackView.depth > 1 ? "\u25C0" : "\u2630"
            font.pixelSize: Qt.application.font.pixelSize * 1.6
            onClicked:
            {
                if ( stackView.depth > 1 )
                {
                    stackView.pop()
                }
                else
                {
                    drawer.open()
                }
            }
        }

        Label
        {
            text: stackView.currentItem.title
            anchors.centerIn: parent
        }
    }

    Drawer
    {
        id: drawer
        width: window.width * 0.4
        height: window.height

        Column
        {
            anchors.fill: parent

            ItemDelegate
            {
                text: qsTr("deck preview generator")
                width: parent.width
                onClicked:
                {
                    stackView.push( deckpreview_generator )
                    drawer.close()
                }
            }

            ItemDelegate
            {
                text: qsTr("Preferences Setting")
                width: parent.width
                onClicked:
                {
                    stackView.push( setting_page )
                    drawer.close()
                }
            }
        }
    }

    StackView
    {
        id: stackView
        anchors.fill: parent
        initialItem:
        Page
        {
            title: qsTr( "sets views" )
            ScrollView
            {
                anchors.fill: parent
                Grid
                {
                    columns: 1
                    TextField
                    {
                        width: stackView.width
                        placeholderText: qsTr("enter filter role string(part or all,only support english,case sensitive)")
                        font.family: customfont.name
                        font.pixelSize: 20
                        onTextChanged:
                        {
                            sets_model.setFilterFixedString( text )
                        }
                    }
                    Repeater
                    {
                        model: sets_model
                        delegate:
                        Row
                        {
                            width: parent.width
                            Downloader
                            {
                                id: downloader
                                onRequest_count_changed:
                                {
                                    card_count.color = "blue"
                                    card_count.text = request_count;
                                }
                            }

                            CheckBox
                            {
                                onClicked:
                                {
                                    if ( checkState == Qt.Checked )
                                    {
                                        if ( func_control.currentIndex == 0 )
                                            downloader.download_set( code )
                                        else
                                            downloader.generator_rankingtable( code )
                                    }
                                }
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text
                            {
                                text: name
                                horizontalAlignment: Text.AlignHCenter
                                font.family: customfont.name
                                width: parent.width*0.5
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text
                            {
                                text: code
                                horizontalAlignment: Text.AlignHCenter
                                font.family: customfont.name
                                width: parent.width*0.2
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text
                            {
                                id: card_count
                                text: count
                                horizontalAlignment: Text.AlignHCenter
                                font.family: customfont.name
                                width: parent.width*0.2
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }
            }
        }
    }
}
