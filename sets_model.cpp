#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "sets_model.h"

constexpr static auto cache_name = "setlist.json";

bool update_setlist_cache( void )
{
    //https://scryfall.com/docs/api/sets/all
    QUrl sets_uri( "https://api.scryfall.com/sets" );
    QJsonObject cache_content;
    QJsonArray set_array;
    do
    {
        QNetworkAccessManager manager;
        QNetworkRequest request = QNetworkRequest( sets_uri );
        //Scryfall provides a REST-like API
        request.setAttribute( QNetworkRequest::FollowRedirectsAttribute , true );
        QEventLoop loop;
        QNetworkReply * reply = manager.get( request );
        QObject::connect( &manager , &QNetworkAccessManager::finished , &loop , &QEventLoop::quit );
        loop.exec();

        QJsonDocument scryfall_doc = QJsonDocument::fromJson( reply->readAll() );
        if ( scryfall_doc.isNull() )
        {
            qWarning() << QString( "scryfall return set list format failure" );
            return false;
        }
        if ( scryfall_doc.isObject() == false )
        {
            return false;
        }
        QJsonObject scryfall_object = scryfall_doc.object();
        if ( ( scryfall_object.contains( "has_more" ) == false ) || ( scryfall_object.contains( "data" ) == false ) )
        {
            qWarning() << QString( "scryfall return set list format failure" );
            return false;
        }
        //https://scryfall.com/docs/api/lists
        //{
        //    ... some key ..
        //    "has_more":bool value
        //    "next_page":string(url),if has_more is true exists
        //    "data":[
        //        {
        //            scryfall set object(https://scryfall.com/docs/api/sets)
        //        },
        //        {
        //            scryfall set object(https://scryfall.com/docs/api/sets)
        //        }
        //        ...
        //    ]
        //}
        QJsonValueRef scryfall_set_data = scryfall_object["data"];
        if ( scryfall_set_data.isArray() == false )
        {
            qWarning() << QString( "scryfall return set list format failure" );
            return false;
        }
        QJsonArray scryfall_sets = scryfall_object["data"].toArray();
        for ( int i = 0 ; i < scryfall_sets.size() ; i++ )
        {
            QJsonObject scryfall_set = scryfall_sets[i].toObject();
            //struct see get_set_list() comment
            QJsonObject cache_set;
            cache_set["code"] = scryfall_set["code"];
            cache_set["card_count"] = scryfall_set["card_count"];
            cache_set["name"] = scryfall_set["name"];
            cache_set["set_type"] = scryfall_set["set_type"];
            set_array.append( cache_set );
        }
        reply->deleteLater();
        if ( scryfall_object["has_more"].toBool() == false )
        {
            break;
        }
        sets_uri = scryfall_object["next_page"].toString();
    }while ( true );

    cache_content["setlist"] = set_array;
    QJsonDocument cache_doc( cache_content );
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
    cache_file.write( cache_doc.toJson() );
    cache_file.close();
    return true;
}

QList<SetInfo> get_set_list( void )
{
    QList<SetInfo> sets;
    if ( QFile::exists( QString( cache_name ) ) == false )
    {
        if ( update_setlist_cache() == false )
        {
            qWarning() << QString( "can not get set list information." );
            return sets;
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
            if ( update_setlist_cache() == false )
            {
                qWarning() << QString( "can not get set list information." );
                return sets;
            }
        }
    }
    QFile cache_file( cache_name );
    if ( cache_file.open( QIODevice::ReadOnly | QIODevice::Text ) == false )
    {
        qWarning() << QString( "can not open:\"%1\"" ).arg( cache_name );
        return sets;
    }

    QJsonDocument cache_doc = QJsonDocument::fromJson( cache_file.readAll() );
    cache_file.close();
    if ( cache_doc.isNull() )
    {
        cache_file.remove();
        qWarning() << QString( "set list cache format failure." );
        return sets;
    }
    if ( cache_doc.isObject() == false )
    {
        cache_file.remove();
        qWarning() << QString( "set list cache format failure." );
        return sets;
    }
    //cache json:
    //{
    //  "setlist":[
    //      {
    //          "code": string,set code
    //          "name": string,set full name
    //          "set_type":string,type enum:MtgTool::SetType
    //          "search_uri":string,return card list json url
    //      },
    //      ...
    //  ]
    //}
    QJsonObject cache_object = cache_doc.object();
    if ( cache_object.contains( "setlist" ) == false )
    {
        cache_file.remove();
        qWarning() << QString( "set list cache format failure." );
        return sets;
    }
    QJsonValue list_node = cache_object["setlist"];
    if ( list_node.isArray() == false )
    {
        cache_file.remove();
        qWarning() << QString( "set list cache format failure." );
        return sets;
    }
    QJsonArray set_array = list_node.toArray();
    for ( int i = 0 ; i < set_array.size() ; i++ )
    {
        QJsonObject set = set_array[i].toObject();
        //serialization set object
        QString code = set["code"].toString();
        QString name = set["name"].toString();
        QString type_str = set["set_type"].toString();
        int count = set["card_count"].toInt();
        sets.append(SetInfo{
            count , code , name,
            static_cast< SetType::TypeEnum >( QMetaEnum::fromType<SetType::TypeEnum>().keyToValue( type_str.toLocal8Bit().data() ) )
        });
    }

    return sets;
}

SetsModel::SetsModel( QObject * /*parent*/ )
{
    this->infos = get_set_list();
}

QVariant SetsModel::data( const QModelIndex& index, int role ) const
{
    if ( index.row() < 0 || index.row() >= this->infos.count() )
        return QVariant();

    const SetInfo &info = this->infos[ index.row() ];
    if ( role == SetName )
    {
        return info.name();
    }
    else if ( role == SetCode )
    {
        return info.code();
    }
    else if ( role == SetType )
    {
        return info.type();
    }
    else if ( role == SetCount )
    {
        return info.count();
    }
    return QVariant();
}

int SetsModel::rowCount( const QModelIndex& /*parent*/ ) const
{
    return this->infos.count();
}

QHash<int, QByteArray> SetsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[SetName] = "name";
    roles[SetCode] = "code";
    roles[SetType] = "type";
    roles[SetCount] = "count";
    return roles;
}

QStringList SetsModel::get_translation()
{
    return QStringList({
        tr("SetName"),
        tr("SetCode"),
        tr("SetType"),
        tr("SetCount")
    });
}

QStringList SetsModel::get_oracle( void )
{
    QStringList oracles;
    int enum_count = QMetaEnum::fromType<SetsModel::EnumContent>().keyCount();

    for ( int i = 0 ; ( enum_count > 0 ) && ( i >= 0 ) ; i++ )
    {
        const char * key = QMetaEnum::fromType<SetsModel::EnumContent>().valueToKey( i );
        if ( key == nullptr )
        {
            continue;
        }
        enum_count--;
        oracles.append( QString( key ) );
    }

    return oracles;
}

SetsFilter::SetsFilter()
{
    ;
}

void SetsFilter::set_fileter_role( int enumvalue )
{
    this->setFilterRole( enumvalue );
}
