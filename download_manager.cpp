#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QVector>

#include <xlnt/xlnt.hpp>

#include "card.h"
#include "download_manager.h"
#include "enums.h"
#include "tool_settings.h"
#include "utils.h"

static bool update_cardlist_cache( const QString& set_code , LanguageEnums::EnumContent lang )
{
    QString lang_code = QString( QMetaEnum::fromType<LanguageEnums::EnumContent>().valueToKey( int( lang ) ) );
    QUrl api_url = QUrl( QString( "https://api.scryfall.com/cards/search?q=s:%1+lang:%2+include:extras" ).arg( set_code ).arg( lang_code ) );
    QString cache_name = QString( "%1.json" ).arg( setcode2legalname(set_code) );
    QFile cache_file( cache_name );
    if ( cache_file.exists() )
    {
        cache_file.remove();
    }
    if ( cache_file.open( QIODevice::WriteOnly | QIODevice::Text ) == false )
    {
        qWarning() << QString( "can not open cache file:'%1'" ).arg( cache_name );
        return false;
    }
    QJsonObject cache_content;
    QJsonArray cards_cache;
    do
    {
        QNetworkAccessManager manager;
        QNetworkRequest request = QNetworkRequest( api_url );
        //Scryfall provides a REST-like API
        request.setAttribute( QNetworkRequest::RedirectPolicyAttribute , true );
        QEventLoop loop;
        QNetworkReply * reply = manager.get( request );
        QObject::connect( &manager , &QNetworkAccessManager::finished , &loop , &QEventLoop::quit );
        loop.exec();

        QJsonDocument scryfall_doc = QJsonDocument::fromJson( reply->readAll() );
        if ( scryfall_doc.isNull() )
        {
            qWarning() << QString( "scryfall return card list format failure,set code:'%1'" ).arg( set_code );
            return false;
        }
        if ( scryfall_doc.isObject() == false )
        {
            qWarning() << QString( "scryfall return card list format failure,set code:'%1'" ).arg( set_code );
            return false;
        }
        QJsonObject scryfall_object = scryfall_doc.object();
        if ( ( scryfall_object.contains( "has_more" ) == false ) || ( scryfall_object.contains( "data" ) == false ) )
        {
            qWarning() << QString( "scryfall return card list format failure,set code:'%1'" ).arg( set_code );
            return false;
        }
        //api: https://scryfall.com/docs/api/cards/search
        //{
        //    ... some key ..
        //    "has_more":bool value
        //    "next_page":string(url),if has_more is true exists
        //    "data":[
        //        {
        //            scryfall card object:https://scryfall.com/docs/api/cards
        //        },
        //        {
        //            scryfall card object:https://scryfall.com/docs/api/cards
        //        }
        //        ...
        //    ]
        //}
        QJsonValueRef scryfall_cards_data = scryfall_object["data"];
        if ( scryfall_cards_data.isArray() == false )
        {
            qWarning() << QString( "scryfall return card list format failure,set code:'%1'" ).arg( set_code );
            return false;
        }
        QJsonArray scryfall_cards = scryfall_object["data"].toArray();
        for ( int i = 0 ; i < scryfall_cards.size() ; i++ )
        {
            QJsonObject scryfall_card = scryfall_cards[i].toObject();
            //struct see SetList::SetList comment
            QJsonObject cardlist_cache;

            //all card common info
            cardlist_cache["oracle_name"] = scryfall_card["name"];
            cardlist_cache["set"] = scryfall_card["set"];
            cardlist_cache["id"] = scryfall_card["collector_number"];
            if ( scryfall_card.contains( "printed_type_line" ) )
            {
                cardlist_cache["print_type"] = scryfall_card["printed_type_line"];
            }
            else
            {
                cardlist_cache["print_type"] = scryfall_card["type_line"];
            }
            cardlist_cache["rarity"] = scryfall_card["rarity"];

            if ( scryfall_card.contains( "card_faces" ) )
            {
                QJsonArray faces = scryfall_card["card_faces"].toArray();
                int face_size = faces.size();
                //non-double face card(e.g:single face split card)
                if ( scryfall_card.contains("image_uris") )
                {
                    QString print_name;
                    QString print_text;
                    QString card_pt;
                    QString mana_cost;
                    for ( int i = 0 ; i < face_size ; i++ )
                    {
                        QJsonObject face = faces[i].toObject();

                        if ( face.contains( "printed_name" ) )
                        {
                            print_name += face["printed_name"].toString();
                        }
                        else
                        {
                            print_name += face["name"].toString();
                        }
                        if ( i + 1 < face_size )
                        {
                            print_name += " // ";
                        }

                        if ( face.contains( "printed_text" ) )
                        {
                            print_text += face["printed_text"].toString();
                        }
                        else if ( face.contains( "oracle_text" ) )
                        {
                            print_text = face["oracle_text"].toString();
                        }
                        if ( ( i + 1 < face_size ) && ( !print_text.isEmpty() ) )
                        {
                            print_text += "\n\n\n\n";
                        }

                        if ( face.contains( "power" ) )
                        {
                            card_pt += QString( "%1/%2" ).arg( face["power"].toString() ).arg( face["toughness"].toString() );
                        }
                        if ( ( i + 1 < face_size ) && ( !card_pt.isEmpty() ) )
                        {
                            card_pt += "\n\n";
                        }

                        mana_cost += face["mana_cost"].toString();
                        if ( i + 1 < face_size )
                        {
                            mana_cost += " // ";
                        }
                    }

                    cardlist_cache["print_name"] = print_name;
                    cardlist_cache["print_text"] = print_text;
                    cardlist_cache["pt"] = card_pt;
                    cardlist_cache["mana_cost"] = mana_cost;
                    cardlist_cache["uris"] = scryfall_card["image_uris"];
                }
                //double face card
                else
                {
                    for ( int i = 0 ; i < face_size ; i++ )
                    {
                        QJsonObject face = faces[i].toObject();
                        cardlist_cache["oracle_name"] = face["name"];
                        if ( face.contains( "printed_name" ) )
                        {
                            cardlist_cache["print_name"] = face["printed_name"];
                        }
                        else
                        {
                            cardlist_cache["print_name"] = face["name"];
                        }
                        if ( face.contains( "printed_text" ) )
                        {
                            cardlist_cache["print_text"] = face["printed_text"];
                        }
                        else if ( face.contains( "oracle_text" ) )
                        {
                           cardlist_cache["print_text"] = face["oracle_text"];
                        }
                        if ( face.contains( "power" ) )
                        {
                            cardlist_cache["pt"] = QString( "%1/%2" ).arg( face["power"].toString() ).arg( face["toughness"].toString() );
                        }
                        else
                        {
                            cardlist_cache["pt"] = "";
                        }
                        cardlist_cache["mana_cost"] = face["mana_cost"];
                        cardlist_cache["uris"] = face["image_uris"];
                        cards_cache.append( cardlist_cache );
                    }
                    continue;
                }
            }
            else
            {
                //single face card info
                if ( scryfall_card.contains( "printed_name" ) )
                {
                    cardlist_cache["print_name"] = scryfall_card["printed_name"];
                }
                else
                {
                    cardlist_cache["print_name"] = "";
                }

                if ( scryfall_card.contains( "printed_text" ) )
                {
                    cardlist_cache["print_text"] = scryfall_card["printed_text"];
                }
                else if ( scryfall_card.contains( "oracle_text" ) )
                {
                    cardlist_cache["print_text"] = scryfall_card["oracle_text"];
                }
                else
                {
                    cardlist_cache["print_text"] = "";
                }

                if ( scryfall_card.contains( "power" ) )
                {
                    QString PT = QString( "%1/%2" ).arg( scryfall_card["power"].toString() ).arg( scryfall_card["toughness"].toString() );
                    cardlist_cache["pt"] = PT;
                }
                else
                {
                    cardlist_cache["pt"] = "";
                }
                cardlist_cache["uris"] = scryfall_card["image_uris"];
                cardlist_cache["mana_cost"] = scryfall_card["mana_cost"];
            }

            cards_cache.append( cardlist_cache );
        }
        reply->deleteLater();
        if ( scryfall_object["has_more"].toBool() == false )
        {
            break;
        }
        api_url = scryfall_object["next_page"].toString();
    }while ( true );

    cache_content["lang"] = lang_code;
    cache_content["cardlist"] = cards_cache;
    QJsonDocument cache_doc( cache_content );
    cache_file.write( cache_doc.toJson() );
    cache_file.close();
    return true;
}

static QVector<Card> get_cardlist( const QString& set_code , LanguageEnums::EnumContent lang )
{
    QVector<Card> cards;
    QString cache_name = QString( "%1.json" ).arg( setcode2legalname(set_code) );
    QFile cache_file( cache_name );
    if ( cache_file.exists() == false )
    {
        if ( update_cardlist_cache( set_code , lang ) == false )
        {
            qWarning() << QString( "can not get the set:'%1' card list information." ).arg( set_code );
            return cards;
        }
    }
    else
    {
        QFileInfo cache_info( cache_name );
        QDateTime last_update = cache_info.lastModified();
        QDateTime now_date = QDateTime::currentDateTime();
        qInfo() << QString( "'%1' cache file last updated %2 days ago" ).arg( set_code ).arg( last_update.daysTo( now_date ) );
        if ( last_update.daysTo( now_date ) >= 1 )
        {
            if ( update_cardlist_cache( set_code , lang ) == false )
            {
                qWarning() << QString( "can not get card list information." );
                return cards;
            }
        }
    }
load_json:
    if ( cache_file.open( QIODevice::ReadOnly | QIODevice::Text ) == false )
    {
        qWarning() << QString( "can not open card list cache,set code:'%1',cache file:'%2'" ).arg( set_code ).arg( cache_name );
        return cards;
    }

    QJsonDocument cache_doc = QJsonDocument::fromJson( cache_file.readAll() );
    cache_file.close();
    if ( cache_doc.isNull() )
    {
        cache_file.remove();
        qWarning() << QString( "card list cache format failure,set code:'%1',cache file:'%2'" ).arg( set_code ).arg( cache_name );
        return cards;
    }
    if ( cache_doc.isObject() == false )
    {
        cache_file.remove();
        qWarning() << QString( "card list cache format failure,set code:'%1',cache file:'%2'" ).arg( set_code ).arg( cache_name );
        return cards;
    }
    //cache json:
    //{
    //  "lang" : "en",
    //  "setlist":[
    //      {
    //          "name": string,card full name
    //          "code": string,set code
    //          "id":string This card’s collector number. Note that collector numbers can contain non-numeric characters, such as letters or ★.
    //          "image_uris":
    //           {
    //              "small":"https://c1.scryfall.com/file/scryfall-cards/small/back/5/d/5d131784-c1a3-463e-a37b-b720af67ab62.jpg?1612316166",
    //              "normal":"https://c1.scryfall.com/file/scryfall-cards/normal/back/5/d/5d131784-c1a3-463e-a37b-b720af67ab62.jpg?1612316166",
    //              "large":"https://c1.scryfall.com/file/scryfall-cards/large/back/5/d/5d131784-c1a3-463e-a37b-b720af67ab62.jpg?1612316166",
    //              "png":"https://c1.scryfall.com/file/scryfall-cards/png/back/5/d/5d131784-c1a3-463e-a37b-b720af67ab62.png?1612316166",
    //              "art_crop":"https://c1.scryfall.com/file/scryfall-cards/art_crop/back/5/d/5d131784-c1a3-463e-a37b-b720af67ab62.jpg?1612316166",
    //              "border_crop":"https://c1.scryfall.com/file/scryfall-cards/border_crop/back/5/d/5d131784-c1a3-463e-a37b-b720af67ab62.jpg?1612316166"
    //          }
    //          ...
    //      },
    //      ...
    //  ]
    //}
    QJsonObject cache_object = cache_doc.object();
    if ( cache_object.contains( "cardlist" ) == false )
    {
        cache_file.remove();
        qWarning() << QString( "card list cache format failure,set code:'%1',cache file:'%2'" ).arg( set_code ).arg( cache_name );
        return cards;
    }
    QString lang_code = QString( QMetaEnum::fromType<LanguageEnums::EnumContent>().valueToKey( int( lang ) ) );
    if (cache_object["lang"].toString() != lang_code)
    {
        if ( update_cardlist_cache( set_code , lang ) == false )
        {
            qWarning() << QString( "can not get card list information." );
            return cards;
        }
        goto load_json;
    }
    QJsonValue list_node = cache_object["cardlist"];
    if ( list_node.isArray() == false )
    {
        cache_file.remove();
        qWarning() << QString( "card list cache format failure,set code:'%1',cache file:'%2'" ).arg( set_code ).arg( cache_name );
        return cards;
    }
    QJsonArray set_array = list_node.toArray();
    for ( int i = 0 ; i < set_array.size() ; i++ )
    {
        QJsonObject cache_card = set_array[i].toObject();
        //serialization set object
        QString oracle_name = cache_card["oracle_name"].toString();
        QString print_name = cache_card["print_name"].toString();
        QString mana_cost = cache_card["mana_cost"].toString();
        QString code = cache_card["set"].toString();
        QString id = cache_card["id"].toString();
        QString print_type = cache_card["print_type"].toString();
        QString print_text = cache_card["print_text"].toString();
        QString rarity = cache_card["rarity"].toString();
        QString pt = cache_card["pt"].toString();
        QJsonObject uris_obj = cache_card["uris"].toObject();
        QMap<QString,QString> scryfall_uris = {
            {
                "small" , uris_obj["small"].toString()
            },
            {
                "normal" , uris_obj["normal"].toString()
            },
            {
                "large" , uris_obj["large"].toString()
            },
            {
                "png" , uris_obj["png"].toString()
            },
            {
                "art_crop" , uris_obj["art_crop"].toString()
            },
            {
                "border_crop" , uris_obj["border_crop"].toString()
            }
        };

        Card new_card(id , code , oracle_name , mana_cost , print_name , print_type , print_text , rarity , pt , scryfall_uris , 0 );
        int start_pos = 0;
        int art_ver = 1;
        while ( ( start_pos = cards.indexOf( new_card , start_pos ) ) != -1 )
        {
            cards[start_pos].set_version(art_ver);
            art_ver++;
            start_pos++;
        }
        // when exist diff art
        if ( art_ver != 1 )
        {
            new_card.set_version(art_ver);
        }
        cards.append( std::move(new_card) );
    }
    return cards;
}

DownloadManager::DownloadManager()
{
    connect( &( this->manager ) , &QNetworkAccessManager::finished , this , &DownloadManager::finished );
}

void DownloadManager::download_card(QUrl local_url, QUrl network_url)
{
    if ( local_url == QUrl() )
        return ;
    if ( network_url == QUrl() )
        return ;

    QFileInfo file_info( local_url.toString() );
    QDir cache_dir = file_info.dir();
    if ( cache_dir.mkpath( "." )  == false )
    {
        qWarning() << QString( "can not create directory:'%1',refuse request" ).arg( cache_dir.absolutePath() );
        return ;
    }
    if ( file_info.exists() == true )
        return ;

    if ( this->localurl_map.contains( local_url ) == true )
        return ;

    QNetworkRequest request = QNetworkRequest( network_url );
    //Scryfall provides a REST-like API
    request.setAttribute( QNetworkRequest::RedirectPolicyAttribute , true );
    QNetworkReply * reply = this->manager.get( request );
    this->localurl_map[ local_url ] = reply;
    emit request_count_changed( this->localurl_map.count() );
}

void DownloadManager::download_set( const QString& set_code )
{
    ToolSetting setting;
    LanguageEnums::EnumContent lang = static_cast<LanguageEnums::EnumContent>( setting.get_image_lanuage() );
    QVector<Card> cards = get_cardlist( set_code , lang );
    for ( auto& card : cards )
    {
        //if local image exist,download_card return immediately,so don't check
        this->download_card( card.local_url() , card.scryfall_url() );
    }
}

void DownloadManager::generate_ratingtable( QString set_code )
{
    const QMap<QString,QString> raritytranslation =
    {
        {
            "common" , tr( "common" )
        },
        {
            "uncommon" , tr( "uncommon" )
        },
        {
            "rare" , tr( "rare" )
        },
        {
            "mythic" , tr( "mythic" )
        }
    };
    const QList<QString> columnnames =
    {
        tr( "set_id" ) , tr( "zh_name" ) , tr( "en_name" ) , tr( "mana_cost" ) , tr( "type" ) ,
        tr( "text" ) , tr( "rarity" ) , tr( "pt" ) , tr( "sealed_rating" ) ,
        tr( "darft_rating" ) , tr( "construct_rating" )
    };
    xlnt::workbook wb;
    xlnt::worksheet ws = wb.active_sheet();
    ws.title("rating");
    ToolSetting setting;
    LanguageEnums::EnumContent lang = static_cast<LanguageEnums::EnumContent>( setting.get_image_lanuage() );
    QVector<Card> cards = get_cardlist( set_code , lang );
    for ( int i = 0 ; i < columnnames.size() ; i++ )
    {
        ws.cell( 1 + i , 1 ).value( columnnames[ i ].toUtf8().toStdString() );
    }
    for ( int i = 0 ; i < cards.size() ; i++ )
    {
        ws.cell( 1  , i + 2 ).value( cards[i].id().toUtf8().toStdString() );
        ws.cell( 2  , i + 2 ).value( cards[i].printed_name().toUtf8().toStdString() );
        ws.cell( 3  , i + 2 ).value( cards[i].name().toUtf8().toStdString() );
        ws.cell( 4  , i + 2 ).value( cards[i].mana_cost().toUtf8().toStdString() );
        ws.cell( 5  , i + 2 ).value( cards[i].printed_type().toUtf8().toStdString() );
        ws.cell( 6  , i + 2 ).value( cards[i].printed_text().toUtf8().toStdString() );
        ws.cell( 7  , i + 2 ).value( raritytranslation[cards[i].printed_rarity()].toUtf8().toStdString() );
        ws.cell( 8  , i + 2 ).value( cards[i].printed_pt().toUtf8().toStdString() );
        ws.cell( 9  , i + 2 ).value( 0 );
        ws.cell( 10 , i + 2 ).value( 0 );
        ws.cell( 11 , i + 2 ).value( 0 );
    }
    QUrl xlsx_url( QString( "%1.xlsx" ).arg(set_code) );
    wb.save( xlsx_url.toString().toUtf8().toStdString() );
    QDesktopServices::openUrl( xlsx_url );
}

int DownloadManager::request_count()
{
    return this->localurl_map.count();
}

void DownloadManager::finished( QNetworkReply * reply )
{
    QUrl download_url = reply->url();
    QUrl local_url = this->localurl_map.key( reply );
    if ( reply->error() )
    {
        qWarning() << QString( "download url:'%1' faild,error message:'%2'" ).arg( download_url.toString() ).arg( reply->errorString() );
    }
    else
    {
        //todo:
        //forge some set need multiple card image name to $(name).1.full.jpg $(name).2.full.jpg...
        //xmage some set need multiple card image name to $(name).$(set_id).full.jpg ...
        QFile image_file( local_url.toString() );
        if ( image_file.open( QIODevice::WriteOnly ) == false )
        {
            qWarning() << QString( "could not open:'%1' for writing,error message:'%2'" ).arg( local_url.toString() ).arg( image_file.errorString() );
        }
        else
        {
            image_file.write( reply->readAll() );
            image_file.close();
        }
    }

    this->localurl_map.remove( local_url );
    reply->deleteLater();
    emit DownloadManager::request_count_changed( this->localurl_map.count() );
    if ( this->localurl_map.empty() )
        emit DownloadManager::download_end();
}
