#include <QClipboard>
#include <QDebug>
#include <QFile>
#include <QGuiApplication>
#include <QTextStream>

#include "deck_parser.h"

DeckParser::DeckParser()
{
    ;
}

Deck * DeckParser::parse_clipboard()
{
    QClipboard * clipboard = QGuiApplication::clipboard();
    QString deck = clipboard->text();

    QStringList lines;
    QTextStream in( &deck );
    while ( in.atEnd() == false )
    {
        QString line = in.readLine();
        lines.append( line );
    }
    Deck * deck_ptr = nullptr;
    try
    {
        deck_ptr = new Deck( lines );
    }
    catch ( const std::invalid_argument& e )
    {
        qWarning() << QString( e.what() );
        return deck_ptr;
    }

    return deck_ptr;
}

Deck * DeckParser::parse_file( const QString &fileuri )
{
    //qml FileDialog return file uri:"file:///****" QFile can't support.
    QUrl url( fileuri );
    QString fileurl = url.toLocalFile();

    QFile deck( fileurl );
    if ( deck.open( QIODevice::ReadOnly | QIODevice::Text ) == false )
    {
        qWarning() << QString( "can not open:\"%1\"" ).arg( fileurl );
        return nullptr;
    }

    QStringList lines;
    QTextStream in( &deck );
    while ( in.atEnd() == false )
    {
        QString line = in.readLine();
        lines.append( line );
    }

    Deck * deck_ptr = nullptr;
    try
    {
        deck_ptr = new Deck( lines );
    }
    catch ( const std::invalid_argument& e )
    {
        qWarning() << QString( e.what() );
        return deck_ptr;
    }

    return deck_ptr;
}
