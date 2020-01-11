#ifndef DECK_H
#define DECK_H

#include <cstdint>

#include <QMap>
#include <QObject>
#include <QStringList>

#include "card.h"
#include "enums.h"

class Deck : public QObject
{
    Q_OBJECT
public:
    Deck();
    //normal deck file,generally less than 100 lines,each line less 30 character,can read all to memory.
    Deck( const QStringList& lines );

    Deck( const Deck& deck );
    Deck( Deck&& deck ) noexcept( true );

    Deck& operator=( const Deck& deck );
    Deck& operator=( Deck&& deck ) noexcept( true );

    //unique card list
    Q_INVOKABLE QList<QObject *> card_list( void );
    Q_INVOKABLE QList<QObject *> commander_list( void );
    Q_INVOKABLE QList<QObject *> main_list( void );
    Q_INVOKABLE QList<QObject *> side_list( void );

    static DeckType inference_type( const QStringList& lines );
private:
    DeckType type;
    QMap<Card,std::uint16_t> commander_cards;
    QMap<Card,std::uint16_t> main_cards;
    QMap<Card,std::uint16_t> sideboard_cards;
};

#endif // DECK_H
