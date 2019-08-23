import QtQuick.Controls 2.5
import QtQuick.Dialogs 1.3 as Dialogs
import QtQuick 2.12
import QtQuick.Window 2.12

Dialogs.Dialog
{
    property int image_width: 105
    property int image_height: 150
    property int dialog_width: Screen.width
    property int dialog_height: Screen.height

    property var main_image_list:
    [
    ]
    property var sideboard_image_list:
    [
    ]

    id: preview_dialog
    width: dialog_width
    height: dialog_height
    modality: Qt.NonModal
    title: qsTr( "deck preview" )
    onRejected:
    {
        console.log("destroy")
        preview_dialog.destroy()
    }

    contentItem:
    ScrollView
    {
        anchors.fill: parent
        Text
        {
            id: main_label
            states:
            [
                State
                {
                    when: main_image_list.length === 0
                    PropertyChanges
                    {
                        target: main_label
                        visible: false
                    }
                }
            ]
            anchors.top: parent.top
            width: parent.width
            font.family: customfont.name
            font.pointSize: 14
            text: qsTr("Main")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        Grid
        {
            id: main_view
            states:
            [
                State
                {
                    when: main_image_list.length === 0
                    PropertyChanges
                    {
                        target: main_view
                        visible: false
                    }
                }
            ]
//            move:
//            Transition
//            {
//                SmoothedAnimation
//                {
//                    properties: "x,y"
//                    easing.type: Easing.Linear
//                    duration: 1000
//                }
//            }
            columns: preview_dialog.width/image_width
            anchors.top: main_label.bottom
            Repeater
            {
                id: main_images
                model:
                ListModel
                {
                    id: main_url_list
                    Component.onCompleted:
                    {
                        for ( var index = 0 ; index < main_image_list.length ; index++ )
                        {
                            main_url_list.append( { "url" : main_image_list[index] } )
                        }
                    }
                }
                delegate:
                DropArea
                {
                    width: preview_dialog.image_width
                    height: preview_dialog.image_height
                    keys: "main"

                    onEntered:
                    {
                        main_images.model.move( drag.source.visualIndex , icon.visualIndex , 1 )
                    }
                    property int visualIndex: index
                    Binding { target: icon; property: "visualIndex"; value: visualIndex }

                    Rectangle
                    {
                        id: icon
                        property int visualIndex: 0
                        width: preview_dialog.image_width
                        height: preview_dialog.image_height
                        anchors
                        {
                            horizontalCenter: parent.horizontalCenter;
                            verticalCenter: parent.verticalCenter
                        }

                        Image
                        {
                            sourceSize.width: preview_dialog.image_width
                            sourceSize.height: preview_dialog.image_height
                            source: modelData
                        }

                        DragHandler
                        {
                            id: dragHandler
                        }

                        Drag.keys: "main"
                        Drag.active: dragHandler.active
                        Drag.source: icon
                        Drag.hotSpot.x: preview_dialog.image_width/2
                        Drag.hotSpot.y: preview_dialog.image_height/2

                        states:
                        [
                            State
                            {
                                when: icon.Drag.active
                                AnchorChanges
                                {
                                    target: icon
                                    anchors.horizontalCenter: undefined
                                    anchors.verticalCenter: undefined
                                }
                            }
                        ]
                    }
                }
            }
        }

        Text
        {
            id: sideboard_label
            states:
            [
                State
                {
                    when: sideboard_image_list.length === 0
                    PropertyChanges
                    {
                        target: sideboard_label
                        visible: false
                    }
                }
            ]
            width: parent.width
            anchors.top: main_view.bottom
            font.family: customfont.name
            font.pointSize: 14
            text: qsTr("Sideboard")
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }
        Grid
        {
            id: sideboard_view
            states:
            [
                State
                {
                    when: sideboard_image_list.length === 0
                    PropertyChanges
                    {
                        target: sideboard_view
                        visible: false
                    }
                }
            ]
//            move:
//            Transition
//            {
//                SmoothedAnimation
//                {
//                    properties: "x,y"
//                    easing.type: Easing.Linear
//                    duration: 1000
//                }
//            }

            columns: preview_dialog.width/image_width
            anchors.top: sideboard_label.bottom
            Repeater
            {
                id: sideboard_images
                model:
                ListModel
                {
                    id: sideboard_url_list
                    Component.onCompleted:
                    {
                        for ( var index = 0 ; index < sideboard_image_list.length ; index++ )
                        {
                            sideboard_url_list.append( { "url" : sideboard_image_list[index] } )
                        }
                    }
                }
                delegate:
                DropArea
                {
                    width: preview_dialog.image_width
                    height: preview_dialog.image_height

                    keys: "side"

                    onEntered:
                    {
                        sideboard_images.model.move( drag.source.visualIndex , side_image.visualIndex , 1 )
                    }
                    property int visualIndex: index
                    Binding { target: side_image; property: "visualIndex"; value: visualIndex }

                    Rectangle
                    {
                        id: side_image
                        property int visualIndex: 0
                        width: preview_dialog.image_width
                        height: preview_dialog.image_height
                        anchors
                        {
                            horizontalCenter: parent.horizontalCenter;
                            verticalCenter: parent.verticalCenter
                        }

                        Image
                        {
                            sourceSize.width: preview_dialog.image_width
                            sourceSize.height: preview_dialog.image_height
                            source: modelData
                        }

                        DragHandler
                        {
                            id: side_dragHandler
                        }

                        Drag.keys: "side"
                        Drag.active: side_dragHandler.active
                        Drag.source: side_image
                        Drag.hotSpot.x: preview_dialog.image_width/2
                        Drag.hotSpot.y: preview_dialog.image_height/2

                        states:
                        [
                            State
                            {
                                when: side_image.Drag.active
                                AnchorChanges
                                {
                                    target: side_image
                                    anchors.horizontalCenter: undefined
                                    anchors.verticalCenter: undefined
                                }
                            }
                        ]
                    }
                }
            }
        }
        width: preview_dialog.width
        contentHeight: main_label.height + sideboard_label.height + main_view.height + sideboard_view.height
    }
}
