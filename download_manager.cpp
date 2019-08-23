#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector>

#include "card.h"
#include "download_manager.h"

static bool update_cardlist_cache( const QString& set_code )
{
    QUrl api_url = QUrl( QString( "https://api.scryfall.com/cards/search?q=s=%1" ).arg( set_code ) );
    QString cache_name = QString( "%1.json" ).arg( set_code );
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
        request.setAttribute( QNetworkRequest::FollowRedirectsAttribute , true );
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
            cardlist_cache["name"] = scryfall_card["name"];
            cardlist_cache["set"] = scryfall_card["set"];
            cardlist_cache["id"] = scryfall_card["collector_number"];
            cards_cache.append( cardlist_cache );
        }
        reply->deleteLater();
        if ( scryfall_object["has_more"].toBool() == false )
        {
            break;
        }
        api_url = scryfall_object["next_page"].toString();
    }while ( true );

    cache_content["cardlist"] = cards_cache;
    QJsonDocument cache_doc( cache_content );
    cache_file.write( cache_doc.toJson() );
    cache_file.close();
    return true;
}

static QVector<Card> get_cardlist( const QString& set_code )
{
    QVector<Card> cards;
    QString cache_name = QString( "%1.json" ).arg( set_code );
    QFile cache_file( cache_name );
    if ( cache_file.exists() == false )
    {
        if ( update_cardlist_cache( set_code ) == false )
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
        qDebug() << QString( "cache file last updated %1 days ago" ).arg( last_update.daysTo( now_date ) );
        if ( last_update.daysTo( now_date ) >= 1 )
        {
            if ( update_cardlist_cache( set_code ) == false )
            {
                qWarning() << QString( "can not get card list information." );
                return cards;
            }
        }
    }
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
    //  "setlist":[
    //      {
    //          "name": string,card full name
    //          "code": string,set code
    //          "id":string This card’s collector number. Note that collector numbers can contain non-numeric characters, such as letters or ★.
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
        QString code = cache_card["set"].toString();
        QString name = cache_card["name"].toString();
        QString id = cache_card["id"].toString();
        cards.append({ id , code , name });
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
    request.setAttribute( QNetworkRequest::FollowRedirectsAttribute , true );
    QNetworkReply * reply = this->manager.get( request );
    this->localurl_map[ local_url ] = reply;
    emit request_count_changed( this->localurl_map.count() );
}

void DownloadManager::download_set( const QString& set_code )
{
    QVector<Card> cards = get_cardlist( set_code );
    for ( auto& card : cards )
    {
        //if local image exist,download_card return immediately,so don't check
        this->download_card( card.local_url() , card.scryfall_url() );
    }
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
