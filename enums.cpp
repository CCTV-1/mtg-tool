#include <QObject>
#include <QMetaEnum>

#include "enums.h"

template<typename ENUM_T>
static QStringList get_qenum_oracle( void )
{
    static_assert(std::is_class<ENUM_T>::value);
    static_assert(std::is_enum<typename ENUM_T::EnumContent>::value);

    QStringList oracles;
    int enum_count = QMetaEnum::fromType<typename ENUM_T::EnumContent>().keyCount();

    for ( int i = 0 ; ( enum_count > 0 ) && ( i >= 0 ) ; i++ )
    {
        const char * key = QMetaEnum::fromType<typename ENUM_T::EnumContent>().valueToKey( i );
        if ( key == nullptr )
        {
            continue;
        }
        enum_count--;
        oracles.append( QString( key ) );
    }

    return oracles;
}

QStringList ImageNameFormatEnums::get_translation()
{
    return this->get_oracle();
}

QStringList ImageNameFormatEnums::get_oracle( void )
{
    return get_qenum_oracle<ImageNameFormatEnums>();
}

QStringList ImageStylesEnums::get_translation()
{
    return QStringList({
        QObject::tr( "a small full card image" ),
        QObject::tr( "a medium-sized full card image" ),
        QObject::tr( "a large full card image" ),
        QObject::tr( "a transparent, rounded full card PNG" ),
        QObject::tr( "a rectangular crop of the card’s art only" ),
        QObject::tr( "a full card image with the rounded corners and the majority of the border cropped off" ),
    });
}

QStringList ImageStylesEnums::get_oracle()
{
    return get_qenum_oracle<ImageStylesEnums>();
}

QStringList LanguageEnums::get_translation()
{
    return QStringList({
        QObject::tr( "English" ),
        QObject::tr( "Spanish" ),
        QObject::tr( "French" ),
        QObject::tr( "German" ),
        QObject::tr( "Italian" ),
        QObject::tr( "Portuguese" ),
        QObject::tr( "Japanese" ),
        QObject::tr( "Korean" ),
        QObject::tr( "Russian" ),
        QObject::tr( "Simplified Chinese" ),
        QObject::tr( "Traditional Chinese" ),
    });
}

QStringList LanguageEnums::get_oracle()
{
    return get_qenum_oracle<LanguageEnums>();
}
