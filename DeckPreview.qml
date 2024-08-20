import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Dialogs 1.3 as Dialogs
import com.mtgtool.utility 1.0

Dialog {
    Downloader { id: deck_downloader }

    function open_preview_dialog( deck )
    {
        if ( deck === null )
            return
        var commander_cards = deck.commander_list()
        var main_cards = deck.main_list()
        var side_cards = deck.side_list()

        var commander_source_urls = []
        for ( let i in commander_cards )
        {
            if ( commander_cards[i].exists_localfile() )
            {
                commander_source_urls.push( commander_cards[i].local_uri().toString() )
            }
            else
            {
                deck_downloader.download_card( commander_cards[i].local_url() , commander_cards[i].scryfall_url() )
                commander_source_urls.push( commander_cards[i].scryfall_url().toString() )
            }
        }

        var main_source_urls = []
        for ( let j in main_cards )
        {
            if ( main_cards[j].exists_localfile() )
            {
                main_source_urls.push( main_cards[j].local_uri().toString() )
            }
            else
            {
                deck_downloader.download_card( main_cards[j].local_url() , main_cards[j].scryfall_url() )
                main_source_urls.push( main_cards[j].scryfall_url().toString() )
            }
        }

        var side_source_urls = []
        for ( let k in side_cards )
        {
            if ( side_cards[k].exists_localfile() )
            {
                side_source_urls.push( side_cards[k].local_uri().toString() )
            }
            else
            {
                deck_downloader.download_card( side_cards[k].local_url() , side_cards[k].scryfall_url() )
                side_source_urls.push( side_cards[k].scryfall_url().toString() )
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
            commander_image_list : commander_source_urls,
            main_image_list : main_source_urls,
            sideboard_image_list : side_source_urls
        })

        preview_dialog.open()
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

}
