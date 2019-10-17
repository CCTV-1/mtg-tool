#ifndef SETS_MODEL_H
#define SETS_MODEL_H

#include <cstdint>

#include <QAbstractListModel>
#include <QList>
#include <QMetaEnum>
#include <QObject>
#include <QStringList>
#include <QSortFilterProxyModel>
#include <QUrl>
#include <QVector>

#include "enums.h"

class SetInfo: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString code READ code CONSTANT)
    Q_PROPERTY(QString type READ type CONSTANT)
public:
    SetInfo():
        SetInfo( 0 , QString( "" ) , QString( "" ) , SetType::TypeEnum::unknown )
    {
        ;
    }
    SetInfo( int count , QString set_code , QString set_name , SetType::TypeEnum set_type ):
        QObject( nullptr ),
        card_count( count ),
        set_code( set_code ),
        set_name( set_name ),
        set_type( set_type )
    {
        ;
    }

    SetInfo( const SetInfo& info ):
        QObject( nullptr ),
        card_count( info.card_count ),
        set_code( info.set_code ),
        set_name( info.set_name ),
        set_type( info.set_type )
    {
        ;
    }

    SetInfo& operator=( const SetInfo& info )
    {
        this->card_count = info.card_count;
        this->set_code = info.set_code;
        this->set_name = info.set_name;
        this->set_type = info.set_type;
        return *this;
    }

    int count( void ) const
    {
        return this->card_count;
    }

    QString code( void ) const
    {
        return this->set_code;
    }
    QString name( void ) const
    {
        return this->set_name;
    }
    QString type( void ) const
    {
        return QString( QMetaEnum::fromType<SetType::TypeEnum>().valueToKey( int( this->set_type ) ) );
    }
private:
    int card_count;
    QString set_code;
    QString set_name;
    SetType::TypeEnum set_type;
};

class SetsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum SetsRoles
    {
        SetName = Qt::UserRole + 1,
        SetCode,
        SetType,
        SetCount
    };
    Q_ENUM( SetsRoles )

    SetsModel( QObject * parent = nullptr );

    int rowCount( const QModelIndex& parent = QModelIndex() ) const;

    QVariant data( const QModelIndex& index , int role = Qt::DisplayRole ) const;

    //support enum model template
    enum class EnumContent:uint32_t
    {
        SetName = Qt::UserRole + 1,
        SetCode,
        SetType,
        SetCount
    };
    Q_ENUM( EnumContent )
    QStringList get_translation( void );
    QStringList get_oracle( void );

protected:
    QHash<int, QByteArray> roleNames() const;
private:
    QList<SetInfo> infos;
};

class SetsFilter : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    SetsFilter();

    Q_INVOKABLE void set_fileter_role( int enumvalue );
};

#endif // SETS_MODEL_H
