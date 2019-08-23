#include <QFileInfo>
#include <QMetaEnum>

#include "card.h"
#include "enums.h"
#include "tool_settings.h"

Card::Card():
    Card( QString() , QString() , QString() )
{
    ;
}

Card::Card( QString id , QString code , QString name ):
    QObject( nullptr ),
    set_id( id ),
    set_code( code.toLower() ),
    card_name( name )
{
    ;
}

Card::Card( const Card& card ):
    QObject( nullptr ),
    set_id( card.set_id ),
    set_code( card.set_code ),
    card_name( card.card_name )
{
    ;
}

Card::Card( Card&& card ) noexcept( true ):
    QObject( nullptr ),
    set_id( std::move( card.set_id ) ),
    set_code( std::move( card.set_code ) ),
    card_name( std::move( card.card_name ) )
{
    ;
}

Card& Card::operator=( const Card& card )
{
    if( &card == this )
        return *this;
    this->set_id = card.set_id;
    this->set_code = card.set_code;
    this->card_name = card.card_name;
    return *this;
}

Card& Card::operator=(Card &&card) noexcept( true )
{
    if( &card == this )
        return *this;
    this->set_id = std::move( card.set_id );
    this->set_code = std::move( card.set_code );
    this->card_name = std::move( card.card_name );
    return *this;
}

bool Card::operator<( const Card& other ) const
{
    return ( this->card_name < other.card_name );
}

bool Card::operator==( const Card& other ) const
{
    return ( this->card_name == other.card_name );
}

bool Card::operator!=( const Card& other ) const
{
    return !( this->operator==( other ) );
}

QString Card::name( void ) const
{
    return this->card_name;
}

QString Card::code( void ) const
{
    return this->set_code;
}

QString Card::to_qstring( void ) const
{
    return QString( "{ set code:'%1', set id:'%2,' card name:'%3' }" ).arg( this->set_code ).arg( this->set_id ).arg( this->card_name );
}

QUrl Card::local_uri( void ) const
{
    QString image_name = this->card_name;
    ToolSetting settings;
    int format = settings.get_image_name_format();
    if ( format == int( ImageNameFormat::FormatEnum::FORGE ) )
    {
        image_name.replace( " // " , "" );
    }
    else if ( format == int( ImageNameFormat::FormatEnum::XMAGE ) )
    {
        image_name.replace( "//" , "-" );
    }
    QString download_dir = settings.get_cache_directory().toString();
    QUrl local_file;
    if ( this->set_code == QString() )
        local_file = QString( "%1/%2.full.jpg" ).arg( download_dir ).arg( image_name );
    else
    {
        local_file = QString( "%1/%2/%3.full.jpg" ).arg( download_dir ).arg( this->set_code ).arg( image_name );
    }

    return local_file;
}

QUrl Card::local_url( void ) const
{
    //qml url exist file:///,but QFileInfo don't need
    return this->local_uri().toLocalFile();
}

QUrl Card::scryfall_url( void ) const
{
    if ( this->card_name == QString() )
    {
        return QUrl();
    }
    ToolSetting settings;
    int lang_value = settings.get_image_lanuage();
    QString lang_code = QString( QMetaEnum::fromType<Languages::LaunguageEnum>().valueToKey( lang_value ) );

    int resoulution_value = settings.get_image_resolution();
    QString resoulution_string = QString( QMetaEnum::fromType<ImageStyles::StylesEnum>().valueToKey( resoulution_value ) );
    if ( this->set_code.isEmpty() )
    {
        return QUrl( QString( "https://api.scryfall.com/cards/named?exact=%1&format=image&version=%2" ).arg( this->card_name ).arg( resoulution_string ) );
    }
    if ( this->set_id != std::numeric_limits<decltype( this->set_id )>::max() )
    {
        //https://scryfall.com/docs/api/cards/collector
        if ( lang_code.isEmpty() )
            return QUrl( QString( "https://api.scryfall.com/cards/%1/%2?format=image&version=%3" ).arg( this->set_code ).arg( this->set_id ).arg( resoulution_string ) );
        else
            return QUrl( QString( "https://api.scryfall.com/cards/%1/%2/%3?format=image&version=%4" ).arg( this->set_code ).arg( this->set_id ).arg( lang_code ).arg( resoulution_string ) );
    }
    return QUrl( QString( "https://api.scryfall.com/cards/named?exact=%1&set=%2&format=image&version=%3" ).arg( this->card_name ).arg( this->set_code ).arg( resoulution_string ) );
}

bool Card::exists_localfile( void ) const
{
    QFileInfo localfile( this->local_url().toString() );
    return localfile.exists();
}
