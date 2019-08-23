#ifndef ENUM_MODEL_H
#define ENUM_MODEL_H

#include <QObject>
#include <QMetaEnum>
#include <QStringList>

class EnumModelBase : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QStringList enums READ enums CONSTANT )
public:
    Q_INVOKABLE virtual int enumvalue_to_index( int )
    {
        return -1;
    }

    Q_INVOKABLE virtual int index_to_enumvalue( int )
    {
        return -1;
    }

    QStringList enums( void )
    {
        return this->enum_strings;
    }

    QStringList enum_strings;
};

template<class ENUM_T>
class EnumModel : public EnumModelBase
{
public:
    EnumModel()
    {
        int enum_count = QMetaEnum::fromType<ENUM_T>().keyCount();

        for ( int i = 0 ; ( enum_count > 0 ) && ( i >= 0 ) ; i++ )
        {
            const char * key = QMetaEnum::fromType<ENUM_T>().valueToKey( i );
            if ( key == nullptr )
            {
                continue;
            }
            enum_count--;
            this->enum_strings.append( QString( key ) );
        }
    }

    int enumvalue_to_index( int enumvalue ) override
    {
        const char * key = QMetaEnum::fromType<ENUM_T>().valueToKey( enumvalue );
        if ( key == nullptr )
        {
            return -1;
        }
        QString enum_name( key );
        int enum_size = this->enum_strings.count();
        for ( int i = 0 ; i < enum_size ; i++ )
        {
            if ( enum_name == this->enum_strings[ i ] )
            {
                return i;
            }
        }
        return -1;
    }

    int index_to_enumvalue( int index ) override
    {
        QString role_name = this->enum_strings[ index ];
        const char * key = role_name.toLocal8Bit().data();
        return QMetaEnum::fromType<ENUM_T>().keyToValue( key );
    }
};

#endif // ENUM_MODEL_H
