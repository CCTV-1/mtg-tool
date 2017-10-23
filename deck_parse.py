#!/usr/bin/env python3
# coding=utf-8

import re

def get_cardname_list(filename)
    cardnamelist = []
    with open(filename , 'r') as deckfile:
        for line in deckfile:
            try:
                match = re.search(r'([0-9]+)\ ([^|]+)\|(.*)',line)
                cardnamelist.append(match.group(2))
            except:
                continue
    return cardnamelist

