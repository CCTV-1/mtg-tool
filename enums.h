#ifndef ENUMS_H
#define ENUMS_H

#include <cstdint>

#include <QObject>
#include <QStringList>

enum class DeckType:std::uint8_t
{
    XMAGE,
    MTGA,
    FORGE,
    GOLDFISH,
    UNKNOWN
};

class ImageNameFormatEnums
{
    Q_GADGET
public:
    enum class EnumContent:std::uint8_t
    {
        FORGE,
        XMAGE
    };
    Q_ENUM( EnumContent );

    QStringList get_translation( void );
    QStringList get_oracle( void );
};

class ImageStylesEnums
{
    Q_GADGET
public:
    enum class EnumContent:std::uint8_t
    {
        small,
        normal,
        large,
        png,
        art_crop,
        border_crop
    };
    Q_ENUM( EnumContent );

    QStringList get_translation( void );
    QStringList get_oracle( void );
};

class LanguageEnums
{
    Q_GADGET
public:
    enum class EnumContent:std::uint8_t
    {
        //English
        en,
        //Spanish
        es,
        //French
        fr,
        //German
        de,
        //Italian
        it,
        //Portuguese
        pt,
        //Japanese
        ja,
        //Korean
        ko,
        //Russian
        ru,
        //Simplified Chinese
        zhs,
        //Traditional Chinese
        zht
        //other language card too little
    };
    Q_ENUM( EnumContent )

    QStringList get_translation( void );
    QStringList get_oracle( void );
};

class SetType
{
    Q_GADGET
public:
    enum class TypeEnum:std::uint8_t
    {
        //A yearly Magic core set (Tenth Edition, etc)
        core,
        //A rotational expansion set in a block (Zendikar, etc)
        expansion,
        //A reprint set that contains no new cards (Modern Masters, etc)
        masters,
        //Masterpiece Series premium foil cards
        masterpiece,
        //From the Vault gift sets
        from_the_vault,
        //Spellbook series gift sets
        spellbook,
        //Premium Deck Series decks
        premium_deck,
        //Duel Decks
        duel_deck,
        //Special draft sets, like Conspiracy and Battlebond
        draft_innovation,
        //Magic Online treasure chest prize sets
        treasure_chest,
        //Commander preconstructed decks
        commander,
        //Planechase sets
        planechase,
        //Archenemy sets
        archenemy,
        //Vanguard card sets
        vanguard,
        //A funny un-set or set with funny promos (Unglued, Happy Holidays, etc)
        funny,
        //A starter/introductory set (Portal, etc)
        starter,
        //A gift box set
        box,
        //A set that contains purely promotional cards
        promo,
        //A set made up of tokens and emblems.
        token,
        //A set made up of gold-bordered, oversize, or trophy cards that are not legal
        memorabilia,
        //sentinel type
        unknown
    };
    Q_ENUM( TypeEnum )
};

#endif // ENUMS_H
