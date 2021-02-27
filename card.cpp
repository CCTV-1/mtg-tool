#include <QDebug>
#include <QFileInfo>
#include <QMetaEnum>

#include "card.h"
#include "enums.h"
#include "tool_settings.h"
#include "utils.h"

Card::Card():
    Card( QString() , QString() , QString() , QString() )
{
    ;
}

Card::Card( QString id , QString code , QString name , QString mana_cost , QString print_name , QString type,
            QString text , QString rarity , QString pt , const QMap<QString,QString>& uris, int version ):
    QObject( nullptr ),
    set_id( id ),
    set_code( code.toLower() ),
    oracle_name( name ),
    cmc( mana_cost ),
    print_name( print_name ),
    print_type( type ),
    print_text( text ),
    rarity( rarity ),
    pt( pt ),
    scryfall_uris( uris ),
    version( version )
{
    ;
}

Card::Card( const Card& card ):
    QObject( nullptr ),
    set_id( card.set_id ),
    set_code( card.set_code ),
    oracle_name( card.oracle_name ),
    cmc( card.cmc ),
    print_name( card.print_name ),
    print_type( card.print_type ),
    print_text( card.print_text ),
    rarity( card.rarity ),
    pt( card.pt ),
    scryfall_uris( card.scryfall_uris ),
    version( card.version )
{
    ;
}

Card::Card( Card&& card ) noexcept( true ):
    QObject( nullptr ),
    set_id( std::move( card.set_id ) ),
    set_code( std::move( card.set_code ) ),
    oracle_name( std::move( card.oracle_name ) ),
    cmc( std::move( card.cmc ) ),
    print_name( std::move( card.print_name ) ),
    print_type( std::move( card.print_type ) ),
    print_text( std::move( card.print_text ) ),
    rarity( std::move( card.rarity ) ),
    pt( std::move( card.pt ) ),
    scryfall_uris( std::move( card.scryfall_uris ) ),
    version( std::move( card.version ) )
{
    ;
}

Card& Card::operator=( const Card& card )
{
    if( &card == this )
        return *this;
    this->set_id = card.set_id;
    this->set_code = card.set_code;
    this->oracle_name = card.oracle_name;
    this->cmc = card.cmc;
    this->print_name = card.print_name;
    this->print_type = card.print_type;
    this->print_text = card.print_text;
    this->rarity = card.rarity;
    this->pt = card.pt;
    this->scryfall_uris = card.scryfall_uris;
    this->version = card.version;
    return *this;
}

Card& Card::operator=(Card &&card) noexcept( true )
{
    if( &card == this )
        return *this;
    this->set_id = std::move( card.set_id );
    this->set_code = std::move( card.set_code );
    this->oracle_name = std::move( card.oracle_name );
    this->cmc = std::move( card.cmc );
    this->print_name = std::move( card.print_name );
    this->print_type = std::move( card.print_type );
    this->print_text = std::move( card.print_text );
    this->rarity = std::move( card.rarity );
    this->pt = std::move( card.pt );
    this->scryfall_uris = std::move( card.scryfall_uris );
    this->version = std::move( card.version );
    return *this;
}

bool Card::operator<( const Card& other ) const
{
    return ( this->oracle_name < other.oracle_name );
}

bool Card::operator==( const Card& other ) const
{
    return ( this->oracle_name == other.oracle_name );
}

bool Card::operator!=( const Card& other ) const
{
    return !( this->operator==( other ) );
}

int Card::get_veriosn( void ) const
{
    return this->version;
}

void Card::set_version( int version )
{
    this->version = version;
}

QString Card::id( void ) const
{
    return this->set_id;
}

QString Card::name( void ) const
{
    return this->oracle_name;
}

QString Card::mana_cost( void ) const
{
    return this->cmc;
}

QString Card::code( void ) const
{
    return this->set_code;
}

QString Card::printed_name( void ) const
{
    return this->print_name;
}

QString Card::printed_type( void ) const
{
    return this->print_type;
}

QString Card::printed_text( void ) const
{
    return this->print_text;
}

QString Card::printed_rarity( void ) const
{
    return this->rarity;
}

QString Card::printed_pt( void ) const
{
    return this->pt;
}

QString Card::to_qstring( void ) const
{
    return QString( "{ set code:'%1', set id:'%2,' card name:'%3' }" ).arg( this->set_code ).arg( this->set_id ).arg( this->oracle_name );
}

QUrl Card::local_uri( void ) const
{
    QString image_name = this->oracle_name;
    ToolSetting settings;
    int format = settings.get_image_name_format();
    QString download_dir = settings.get_cache_directory().toString();
    QUrl local_file;
    if ( format == int( ImageNameFormatEnums::EnumContent::FORGE ) )
    {
        image_name.replace( " // " , "" );
        if ( this->set_code == QString() )
        {
            if ( this->version == 0 )
            {
                local_file = QString( "%1/%2.full.jpg" ).arg( download_dir ).arg( image_name );
            }
            else
            {
                local_file = QString( "%1/%2%3.full.jpg" ).arg( download_dir ).arg( image_name ).arg( this->version );
            }
        }
        else
        {
            if ( this->version == 0 )
            {
                local_file = QString( "%1/%2/%3.full.jpg" ).arg( download_dir ).arg( setcode2legalname( this->set_code ) ).arg( image_name );
            }
            else
            {
                local_file = QString( "%1/%2/%3%4.full.jpg" ).arg( download_dir ).arg( setcode2legalname( this->set_code ) )
                    .arg( image_name ).arg( this->version );
            }
        }
    }
    else if ( format == int( ImageNameFormatEnums::EnumContent::XMAGE ) )
    {
        image_name.replace( "//" , "-" );
        if ( this->set_code == QString() )
        {
            if ( this->version == 0 )
            {
                local_file = QString( "%1/%2.full.jpg" ).arg( download_dir ).arg( image_name );
            }
            else
            {
                local_file = QString( "%1/%2.%3.full.jpg" ).arg( download_dir ).arg( image_name ).arg( this->set_id );
            }
        }
        else
        {
            if ( this->version == 0 )
            {
                local_file = QString( "%1/%2/%3.full.jpg" ).arg( download_dir ).arg( setcode2legalname( this->set_code ) ).arg( image_name );
            }
            else
            {
                local_file = QString( "%1/%2/%3.%4.full.jpg" ).arg( download_dir ).arg( setcode2legalname( this->set_code ) )
                    .arg( image_name ).arg( this->set_id );
            }
        }
    }
    else
    {
        qWarning() << "unknown image format!";
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
    if ( this->oracle_name == QString() )
    {
        return QUrl();
    }
    ToolSetting settings;
    int lang_value = settings.get_image_lanuage();
    QString lang_code = QString( QMetaEnum::fromType<LanguageEnums::EnumContent>().valueToKey( lang_value ) );

    int resoulution_value = settings.get_image_resolution();
    QString resoulution_string = QString( QMetaEnum::fromType<ImageStylesEnums::EnumContent>().valueToKey( resoulution_value ) );
    if ( this->scryfall_uris.contains(resoulution_string.toLower()) )
    {
        return this->scryfall_uris[resoulution_string.toLower()];
    }
    if ( this->set_code.isEmpty() )
    {
        return QUrl( QString( "https://api.scryfall.com/cards/named?exact=%1&format=image&version=%2" ).arg( this->oracle_name ).arg( resoulution_string ) );
    }
    if ( this->set_id != std::numeric_limits<decltype( this->set_id )>::max() )
    {
        //https://scryfall.com/docs/api/cards/collector
        if ( lang_code.isEmpty() )
            return QUrl( QString( "https://api.scryfall.com/cards/%1/%2?format=image&version=%3" ).arg( this->set_code ).arg( this->set_id ).arg( resoulution_string ) );
        else
            return QUrl( QString( "https://api.scryfall.com/cards/%1/%2/%3?format=image&version=%4" ).arg( this->set_code ).arg( this->set_id ).arg( lang_code ).arg( resoulution_string ) );
    }
    return QUrl( QString( "https://api.scryfall.com/cards/named?exact=%1&set=%2&format=image&version=%3" ).arg( this->oracle_name ).arg( this->set_code ).arg( resoulution_string ) );
}

bool Card::exists_localfile( void ) const
{
    QFileInfo localfile( this->local_url().toString() );
    return localfile.exists();
}
