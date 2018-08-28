#!/usr/bin/python3
# coding=utf-8

"""Get card image in https://scryfall.com"""

import getopt
import logging
import math
import os
import re
import sys
from multiprocessing import Pool

import requests


def helps():
    """get help information"""
    print(
        '--getsetlist get Scryfall.com supported setlist and each set shortname\n'
        '--getsetinfo=[set shortname] get set cards list\n'
        '--downloadset=[set shortname] download set all card image\n'
        '--downloaddeck=[deckname] download deck content all card image\n'
        '--getcardinfo=[cardname] get card info(usage english name is best)\n'
        '--downloadcard=[cardname] download card image\
,but not support download reprint card history image'
    )


def getsetlist():
    """get Scryfall.com supported series list and each set shortname"""
    try:
        resp = requests.get(
            'https://api.scryfall.com/sets', timeout=13)
        if resp.status_code != 200:
            return None
        serieslist = resp.json()['data']
        return serieslist
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        logging.info('Get setlist time out\n')
    except (AttributeError, KeyError):
        logging.info('can\'t get setlist\n')


def getcardlist(deckname):
    """read deck file,get card list"""
    cardnamelist = []
    with open(deckname, 'r') as deckfile:
        for line in deckfile:
            try:
                match = re.search(r'([0-9]+)\ ([^|]+)\|(.*)', line)
                cardname = match.group(2)
                cardnamelist.append({'name': cardname})
            except (IndexError, AttributeError):
                continue
    return cardnamelist


def getsetinfo(setshortname, lang='en'):
    """Get Series information"""
    try:
        resp = requests.get(
            'https://api.scryfall.com/sets/{0}'.format(setshortname), timeout=13)
        if resp.status_code != 200:
            return None
        seriessize = resp.json()['card_count']
        seriescode = resp.json()['code']

        cardobjs = []
        for cardid in range(1, seriessize + 1):
            cardobjs.append((seriescode, cardid, lang))

        with Pool(processes=8) as P:
            cardsinfo = P.map(getcardinfo_fromid, cardobjs)

        return cardsinfo
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        logging.info("Get set:'%s' card list time out", setshortname)
    except (AttributeError, TypeError, KeyError):
        logging.info("Set:'%s' information obtained is wrong\n", setshortname)


def getcardinfo_fromid(cardobj):
    try:
        resp = requests.get(
            # serie_code,serie_id,cardlang
            'https://api.scryfall.com/cards/{0}/{1}/{2}'.format(cardobj[0], cardobj[1], cardobj[2]), timeout=13)
        if resp.status_code != 200:
            return None
        cardinfo = resp.json()
        return cardinfo
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        logging.info("Get Card: '%s':'%s' info time out",
                     cardobj[0], cardobj[1])
    except (AttributeError, TypeError, KeyError):
        logging.info("Get Card:'%s':'%s' Info Failure\n",
                     cardobj[0], cardobj[1])
    except (IndexError):
        logging.info("cardobj:'%s' content format faliure\n", str(cardobj))


def getcardinfo_fromname(cardname):
    try:
        resp = requests.get(
            'https://api.scryfall.com/cards/named?exact={0}'.format(cardname), timeout=13)
        if resp.status_code != 200:
            return None
        cardinfo = resp.json()
        return cardinfo
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        logging.info("Get Card:'%s' info time out", cardname)
    except (AttributeError, TypeError, KeyError):
        logging.info("Get Card:'%s' Info Failure\n", cardname)


def downloadset(setname, lang='en'):
    cardsinfo = getsetinfo(setname, lang)
    if os.path.exists('./' + setname) is False:
        os.mkdir('./' + setname)
    os.chdir('./' + setname)
    P = Pool(processes=8)
    for cardobj in cardsinfo:
        P.apply_async(downloadcard, args=(
            cardobj, ))
    P.close()
    P.join()
    os.chdir('../')


def downloaddeck(deckname):
    cardlist = getcardlist(deckname)
    if os.path.exists('./images') is False:
        os.mkdir('./images')
    os.chdir('./images')
    P = Pool(processes=8)
    for cardobj in cardlist:
        P.apply_async(downloadcard, args=(
            cardobj, False))
    P.close()
    P.join()
    os.chdir('../')


def downloadcard(cardobj, rename_flags=True):
    download_descptions = []

    try:
        download_descptions.append(
            (cardobj['name'], cardobj['image_uris']['large']))
    except (KeyError):
        for card_face in cardobj['card_faces']:
            download_descptions.append(
                (card_face['name'], card_face['image_uris']['large']))

    for download_descption in download_descptions:
        if rename_flags == True:
            open_flags = 'xb'
        else:
            open_flags = 'wb'
        retry_flag = 8

        try:
            renamecount = 0
            basecardname = download_descption[0]
            cardname = basecardname
            image_object = requests.get(download_descption[1], timeout=13)
            if image_object.status_code != 200:
                logging.info("get card:'%s' image fail\n", cardname)
                return
            while retry_flag:
                try:
                    # x mode in python3.3+
                    open(cardname + '.full.jpg',
                         open_flags).write(image_object.content)
                    logging.info("Download card:'%s' success", cardname)
                    break
                except FileNotFoundError:
                    cardname = cardname.replace(' // ', '')
                    retry_flag -= 1
                    logging.info("Cookiescard:'%s' rename to:'%s'\n",
                                 basecardname, cardname)
                except FileExistsError:
                    # rename base land
                    renamecount += 1
                    retry_flag -= 1
                    cardname = basecardname + str(renamecount)
        except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
            logging.info(
                "Download Card '%s' request timeout stop downloading!\n", cardname)
        except (AttributeError, TypeError, KeyError):
            logging.info(
                "The card:'%s' information obtained is wrong\n", cardname)


def main():
    logging.basicConfig(filename='GetImage.log',
                        filemode='w', level=logging.INFO)
    try:
        options, args = getopt.getopt(sys.argv[1:], '', longopts=[
            'help', 'getsetlist', 'getsetinfo=', 'getcardinfo=', 'downloadset=', 'downloaddeck=', 'downloadcard='])
        args = args  # wipe off unused warning
        for name, value in options:
            if name in '--help':
                helps()
                continue

            if name in '--getsetlist':
                setlist = getsetlist()
                print('support set is:\n')
                for setobj in setlist:
                    print("{0}({1})".format(setobj['name'], setobj['code']))
                continue

            if name in '--getsetinfo':
                setshortname = value  # 'akh
                cardsinfo = getsetinfo(setshortname)
                for cardobj in cardsinfo:
                    print("{0}\t{1}".format(
                        cardobj['name'], cardobj['mana_cost']))
                continue

            if name in '--getcardinfo':
                cardinfo = getcardinfo_fromname(value)
                print("{0}:{1}".format('name', cardinfo['name']))
                print("{0}:{1}".format('mana cost', cardinfo['mana_cost']))
                print("{0}:{1}".format('card type', cardinfo['type_line']))
                print("{0}:{1}".format(
                    'card content', cardinfo['oracle_text']))
                continue

            if name in '--downloadset':
                setshortname = value
                downloadset(setshortname)
                continue

            if name in '--downloaddeck':
                deckname = value
                downloaddeck(deckname)
                continue

            if name in '--downloadcard':
                cardname = value
                cardinfo = getcardinfo_fromname(cardname)
                downloadcard(cardinfo, False)
                continue

    except getopt.GetoptError:
        helps()
    except (AttributeError, TypeError, KeyError):
        print('get series or card information format failure')


if __name__ == '__main__':
    main()
