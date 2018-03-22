#!/usr/bin/env python3
# coding=utf-8

"""
get deck card image in https://scryfall.com/
request api see https://scryfall.com/docs/api
"""

import logging
import os
import re
import time
from multiprocessing import Pool

import requests


def get_cardobj(cardname):
    """get card image url and name"""
    # scryfall api rate Limits: 10 requests per second on average
    # this function don't usage multiprocessing/Thread call so don't sleep
    cardobj = []
    resp = requests.get('https://api.scryfall.com/cards/named?exact={0}&format=json'
                        .format(cardname), timeout=13)
    info_json = resp.json()
    cardfaces = info_json.get('cardfaces')
    if cardfaces is not None:
        for card_face in cardfaces:
            name = card_face.get('name')
            url = card_face.get('image_uris').get('normal')
            cardobj.append((name, url))
    else:
        name = info_json.get('name')
        url = info_json.get('image_uris').get('normal')
        cardobj.append((name, url))
    return cardobj


def get_cardlist(deckname):
    """read deck file,get cards list"""
    cardnamelist = []
    with open(deckname, 'r') as deckfile:
        for line in deckfile:
            try:
                match = re.search(r'([0-9]+)\ ([^|]+)\|(.*)', line)
                cardname = match.group(2)
                cardnamelist.append(cardname)
            except (IndexError, AttributeError):
                continue
    return cardnamelist


def downloadimage(cardname, cardurl):
    """download card image"""
    # scryfall api rate Limits: 10 requests per second on average
    # this function usage multiprocessing/Thread call so sleep 0.1
    time.sleep(0.1)
    try:
        imageobject = requests.get(cardurl, timeout=13)
        # x mode in python3.3+
        open(cardname + '.full.jpg', 'xb').write(imageobject.content)
        logging.info("Download card:%s success,the url is:%s",
                     cardname, cardurl)
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        logging.info(
            "Download Card %s failure,timeout stop downloading!", cardname)
    except (AttributeError, TypeError, KeyError):
        logging.info(
            "Download Card %s failure,card information wrong", cardname)
    except (FileExistsError):
        logging.info("Download card:%s failure,the image exists", cardname)


def main():
    """some preparation work"""
    logging.basicConfig(filename='GetImage.log',
                        filemode='w', level=logging.INFO)

    deckname = input('you plan download card images deck file name:')
    cardsobject = []
    print("prase deck:%s start", deckname)
    cardlist = get_cardlist(deckname)
    print("prase deck:%s end", deckname)
    print('get card images start')
    for cardname in cardlist:
        cardobjects = get_cardobj(cardname)
        for cardobject in cardobjects:
            cardsobject.append(cardobject)
    print('get card images end')

    if os.path.exists('./images') is False:
        os.mkdir('./images')
    os.chdir('./images')
    processes = Pool(processes=4)
    for card in cardsobject:
        processes.apply_async(downloadimage, args=(
            card[0], card[1], ))
    processes.close()
    print('card image download start,image total:%d', len(cardsobject))
    processes.join()
    print('card image download end')
    os.chdir('./..')


if __name__ == '__main__':
    main()
