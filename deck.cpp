#include <QRegExp>
#include <QSet>
#include <QString>

#include "deck.h"

//1 [UST:214] Swamp
//1 [BFZ:266a] Mountain
//SB: 1 [UST:214] Swamp
static const QRegExp xmage_deck_reg( R"(^(SB: )?(\d+) \[([^:]*):([0-9a-z\]]+)\] ([^\r\n]+))" );
//1 Mountain (BFZ) 266a
//2 Tyrant's Scorn (WAR) 225
static const QRegExp mtga_deck_reg( R"(^([0-9]+) ([^\(^\)]+) \(([^ ]+)\) ([0-9a-z]+))" );
//1 Swamp|UST|1
static const QRegExp forge_deck_reg( R"(^([0-9]+) ([^|]+)\|([^|]+))" );
//1 Swamp
static const QRegExp goldfish_deck_reg( R"(^([0-9]+) ([^|\r\n]+))" );

Deck::Deck():
    QObject( nullptr ),
    type( DeckType::UNKNOWN )
{
    ;
}

Deck::Deck( const Deck& deck ):
    QObject( nullptr ),
    type( deck.type ),
    main_cards( deck.main_cards ),
    sideboard_cards( deck.sideboard_cards )
{
    ;
}

Deck::Deck( Deck&& deck ) noexcept( true ):
    QObject( nullptr ),
    type( std::move( deck.type ) ),
    main_cards( std::move( deck.main_cards ) ),
    sideboard_cards( std::move( deck.sideboard_cards ) )
{
    ;
}

Deck& Deck::operator=( const Deck& deck )
{
    if ( &deck == this )
    {
        return *this;
    }

    this->type = deck.type;
    this->main_cards = deck.main_cards;
    this->sideboard_cards = deck.sideboard_cards;
    return *this;
}

Deck& Deck::operator=( Deck&& deck ) noexcept( true )
{
    if ( &deck == this )
    {
        return *this;
    }

    this->type = std::move( deck.type );
    this->main_cards = std::move( deck.main_cards );
    this->sideboard_cards = std::move( deck.sideboard_cards );
    return *this;
}

Deck::Deck( const QStringList& lines )
{
    this->type = this->inference_type( lines );

    decltype( this->main_cards )* list_prt = &( this->main_cards );
    switch ( this->type )
    {
        case DeckType::XMAGE:
        {
            for ( auto line : lines )
            {
                if ( xmage_deck_reg.indexIn( line , 0 ) != -1 )
                {
                    if ( xmage_deck_reg.cap(1).isEmpty() == false )
                    {
                        list_prt = &( this->sideboard_cards );
                    }
                    QString set_id = xmage_deck_reg.cap(4);
                    QString set_code = xmage_deck_reg.cap(3);
                    QString card_name = xmage_deck_reg.cap(5);
                    ( *list_prt )[ Card( set_id , set_code , card_name ) ] += xmage_deck_reg.cap(2).toInt();
                }
            }
            break;
        }
        case DeckType::MTGA:
        {
            decltype (this->main_cards) discard_cards;
            for ( auto line : lines )
            {
                if ( mtga_deck_reg.indexIn( line , 0 ) != -1 )
                {
                    QString set_id = mtga_deck_reg.cap(4);
                    QString set_code = mtga_deck_reg.cap(3);
                    QString card_name = mtga_deck_reg.cap(2);

                    if ( set_code.toUpper() == "DAR" )
                    {
                        set_code = "DOM";
                    }
                    ( *list_prt )[ Card( set_id , set_code , card_name ) ] += mtga_deck_reg.cap(1).toInt();
                }
                else if ( line == QString( "Commander" ) )
                {
                    list_prt = &( this->commander_cards );
                }
                else if ( line == QString( "Deck" ) )
                {
                    list_prt = &( this->main_cards );
                }
                else if ( line == QString( "Sideboard" ) )
                {
                    list_prt = &( this->sideboard_cards );
                }
                if ( line == QString( "Companion" ) )
                {
                    list_prt = &discard_cards;
                }
            }
            break;
        }
        case DeckType::FORGE:
        {
            for ( auto line : lines )
            {
                if ( forge_deck_reg.indexIn( line , 0 ) != -1 )
                {
                    QString set_id = QString();
                    QString set_code = forge_deck_reg.cap(3);
                    QString card_name = forge_deck_reg.cap(2);
                    ( *list_prt )[ Card( set_id , set_code , card_name ) ] += forge_deck_reg.cap(1).toInt();
                }
                else if ( line.toLower().contains( "sideboard" ) )
                {
                    list_prt = &( this->sideboard_cards );
                }
            }
            break;
        }
        case DeckType::GOLDFISH:
        {
            for ( auto line : lines )
            {
                if ( goldfish_deck_reg.indexIn( line , 0 ) != -1 )
                {
                    QString set_id = QString();
                    QString set_code = QString();
                    QString card_name = goldfish_deck_reg.cap(2);
                    ( *list_prt )[ Card( set_id , set_code , card_name ) ] += goldfish_deck_reg.cap(1).toInt();
                }
                else if ( line == QString( "" ) )
                {
                    list_prt = &( this->sideboard_cards );
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

QList<QObject *> Deck::card_list( void )
{
    QList<QObject *> cards;
    QSet<Card> card_set;
    for ( auto card : this->main_cards.keys() )
    {
        card_set.insert( card );
    }
    for ( auto card : this->sideboard_cards.keys() )
    {
        card_set.insert( card );
    }

    for( auto& card : card_set )
    {
        cards.append( new Card( card ) );
    }
    return cards;
}

QList<QObject *> Deck::commander_list( void )
{
    QList<QObject *> cards;
    for ( auto card : this->commander_cards.keys() )
    {
        for ( int i = 0 ; i < this->commander_cards.value( card ) ; i++ )
            cards.append( new Card( card ) );
    }

    return cards;
}

QList<QObject *> Deck::main_list( void )
{
    QList<QObject *> cards;
    for ( auto card : this->main_cards.keys() )
    {
        for ( int i = 0 ; i < this->main_cards.value( card ) ; i++ )
            cards.append( new Card( card ) );
    }

    return cards;
}

QList<QObject *> Deck::side_list( void )
{
    QList<QObject *> cards;
    for ( auto card : this->sideboard_cards.keys() )
    {
        for ( int i = 0 ; i < this->sideboard_cards.value( card ) ; i++ )
            cards.append( new Card( card ) );
    }

    return cards;
}

DeckType Deck::inference_type( const QStringList& lines )
{
    DeckType type = DeckType::UNKNOWN;
    for( auto line : lines )
    {
        //fast check,effective line only first character: '1'-'9' or 'S'
        if ( ( line[0].isDigit() == false ) && ( line[0] != 'S' ) )
        {
            continue;
        }
        if ( xmage_deck_reg.indexIn( line , 0 ) != -1 )
        {
            type = DeckType::XMAGE;
            return type;
        }
        if ( mtga_deck_reg.indexIn( line , 0 ) != -1 )
        {
            type = DeckType::MTGA;
            return type;
        }
        if ( forge_deck_reg.indexIn( line , 0 ) != -1 )
        {
            type = DeckType::FORGE;
            return type;
        }
        if ( goldfish_deck_reg.indexIn( line , 0 ) != -1 )
        {
            type = DeckType::GOLDFISH;
            return type;
        }
    }

    return type;
}
