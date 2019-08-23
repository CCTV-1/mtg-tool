#ifndef CARD_H
#define CARD_H

#include <QObject>
#include <QString>
#include <QUrl>

class Card : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString code READ code CONSTANT)
public:
    Card();
    Card( QString id , QString code , QString name );
    Card( const Card& card );
    Card( Card&& card ) noexcept( true );

    Card& operator=( const Card& card );
    Card& operator=( Card &&card ) noexcept( true );
    bool operator<( const Card& other ) const;
    bool operator==( const Card& other ) const;
    bool operator!=( const Card& other ) const;

    QString name( void ) const;
    QString code( void ) const;
    Q_INVOKABLE QUrl local_uri( void ) const;
    Q_INVOKABLE QUrl local_url( void ) const;
    Q_INVOKABLE QUrl scryfall_url( void ) const;
    Q_INVOKABLE bool exists_localfile( void ) const;
    QString to_qstring( void ) const;

private:
    QString set_id;
    QString set_code;
    QString card_name;
};
Q_DECLARE_METATYPE(Card)

inline uint qHash( const Card& card , uint seed )
{
    return qHash( QString( "%1/%2" ).arg( card.code() ).arg( card.name() ) , seed );
}

#endif // CARD_H
