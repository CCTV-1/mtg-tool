#pragma once
#ifndef DECKPRASE_H
#define DECKPRASE_H

#include <glib.h>

#include "commontypes.h"

struct CardObject
{
    enum CardLocal card_local;
    gchar * cardname;
    gchar * cardseries;
    gint32 cardid;
    gint32 cardnumber;
};

GSList * get_cardlist( const gchar * deckfilename );

void delete_cardlist( GSList * cardlist );

#endif