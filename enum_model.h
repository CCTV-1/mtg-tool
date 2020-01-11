#ifndef ENUM_MODEL_H
#define ENUM_MODEL_H

#include <type_traits>

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
        return this->translation_strings;
    }

    QStringList translation_strings;
};

template<class ENUM_T>
class EnumModel : public EnumModelBase
{
    static_assert(std::is_class<ENUM_T>::value);
    static_assert(std::is_default_constructible<ENUM_T>::value);
    static_assert(std::is_enum<typename ENUM_T::EnumContent>::value);
    static_assert(std::is_member_function_pointer<decltype(&ENUM_T::get_translation)>::value);

public:
    EnumModel()
    {
        this->translation_strings = ENUM_T().get_translation();
        this->oracle_strings = ENUM_T().get_oracle();
    }

    int enumvalue_to_index( int enumvalue ) override
    {
        const char * key = QMetaEnum::fromType<typename ENUM_T::EnumContent>().valueToKey( enumvalue );
        if ( key == nullptr )
        {
            return -1;
        }
        QString enum_name( key );
        int enum_size = this->oracle_strings.count();
        for ( int i = 0 ; i < enum_size ; i++ )
        {
            if ( enum_name == this->oracle_strings[ i ] )
            {
                return i;
            }
        }
        return -1;
    }

    int index_to_enumvalue( int index ) override
    {
        QString role_name = this->oracle_strings[ index ];
        const char * key = role_name.toLocal8Bit().data();
        return QMetaEnum::fromType<typename ENUM_T::EnumContent>().keyToValue( key );
    }

private:
    QStringList oracle_strings;
};

#endif // ENUM_MODEL_H
