#ifndef DECK_PARSER_H
#define DECK_PARSER_H

#include <QObject>

#include "deck.h"

class DeckParser : public QObject
{
    Q_OBJECT
public:
    DeckParser();

    Q_INVOKABLE Deck * parse_clipboard();
    Q_INVOKABLE Deck * parse_file( const QString& fileuri );
};

#endif // DECK_PARSE_H
