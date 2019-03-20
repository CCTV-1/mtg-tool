#!/usr/bin/python3
# coding=utf-8

"""get card image in https://scryfall.com"""

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
        '--getsetlist get scryfall.com supported setlist and each set shortname\n'
        '--getsetinfo=[set shortname] get set cards list\n'
        '--getcardinfo=[cardname] get card info(usage english name is best)\n'
        '--downloadformat=[format name] download format all card image\n'
        '--downloadset=[set shortname] download set all card image\n'
        '--downloadcube=[cube name] download cube all card image\n'
        '--downloaddeck=[deckname] download deck content all card image\n'
        '--downloadcard=[cardname] download card image\
,but not support download reprint card history image'
    )


def getsetlist():
    """get scryfall.com supported series list and each set shortname"""
    try:
        resp = requests.get(
            'https://api.scryfall.com/sets', timeout=13)
        if resp.status_code != 200:
            return None
        serieslist = resp.json()['data']
        return serieslist
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        logging.info('get setlist time out\n')
    except (AttributeError, KeyError):
        logging.info('can\'t get setlist\n')


def getcardlist(deckname):
    """read deck file,get card list"""
    cardnamelist = []
    with open(deckname, 'r') as deckfile:
        for line in deckfile:
            try:
                match = re.search(r'([0-9]+)\ ([^|^\r^\n]+)', line)
                cardname = match.group(2)
                cardnamelist.append(cardname)
            except (IndexError, AttributeError):
                continue
    return cardnamelist


def get_queue_cardlist(queue_type, queue_content):
    """get a queue result cards information"""
    try:
        resp = requests.get(
            'https://api.scryfall.com/cards/search?q={0}:{1}'.format(queue_type, queue_content), timeout=13)
        if resp.status_code != 200:
            return None
        info_content = resp.json()
        cardsinfo = []
        for cardinfo in info_content['data']:
            cardsinfo.append(cardinfo)
        has_more = info_content['has_more']

        while has_more != False:
            resp = requests.get(
                info_content.get('next_page'), timeout=13)
            info_content = resp.json()
            for cardinfo in info_content['data']:
                cardsinfo.append(cardinfo)
            has_more = info_content['has_more']

        return cardsinfo
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        logging.info("queue %s:'%s' card list time out",
                     queue_type, queue_content)
    except (AttributeError, TypeError, KeyError):
        logging.info("queue %s:'%s' list format is wrong\n",
                     queue_type, queue_content)


def getformatinfo(formatname, lang='en'):
    """get format information"""
    return get_queue_cardlist("format", formatname)


def getsetinfo(setshortname, lang='en'):
    """get series information"""
    return get_queue_cardlist("s", setshortname)


def getcubeinfo(setshortname, lang='en'):
    """get cube information"""
    return get_queue_cardlist("cube", setshortname)


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


def donwload_cardlist(dir_name, cardlist):
    if os.path.exists('./' + dir_name) is False:
        os.mkdir('./' + dir_name)
    os.chdir('./' + dir_name)
    P = Pool(processes=8)
    for cardobj in cardlist:
        P.apply_async(downloadcard, args=(
            cardobj, ))
    P.close()
    P.join()
    os.chdir('../')


def downloadformat(formatname, lang='en'):
    cardsinfo = getformatinfo(formatname, lang)
    donwload_cardlist(formatname, cardsinfo)


def downloadset(setname, lang='en'):
    cardsinfo = getsetinfo(setname, lang)
    donwload_cardlist(setname, cardsinfo)


def downloadcube(cubename, lang='en'):
    cardsinfo = getcubeinfo(cubename, lang)
    donwload_cardlist(cubename, cardsinfo)


def downloaddeck(deckname, lang='en'):
    cardlist = getcardlist(deckname)
    with Pool(processes=8) as P:
        cardsinfo = P.map(getcardinfo_fromname, cardlist)

    donwload_cardlist("{0}_images".format(deckname), cardsinfo)


def downloadcard(cardobj, rename_flags=True, resolution='large', filename_format='xmage'):
    download_descptions = []

    try:
        download_descptions.append(
            (cardobj['name'], cardobj['image_uris'][resolution]))
    except (KeyError):
        for card_face in cardobj['card_faces']:
            download_descptions.append(
                (card_face['name'], card_face['image_uris'][resolution]))

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
                    if filename_format == 'forge':
                        cardname = cardname.replace(' // ', '')
                    elif filename_format == 'xmage':
                        cardname = cardname.replace('//', '-')
                    else:
                        pass
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
            'help', 'getsetlist', 'getsetinfo=', 'getcardinfo=', 'downloadformat=', 'downloadcube=', 'downloadset=', 'downloaddeck=', 'downloadcard='])
        args = args  # wipe off unused warning
        for name, value in options:
            if name == '--help':
                helps()
                continue

            if name == '--getsetlist':
                setlist = getsetlist()
                print('support set is:\n')
                for setobj in setlist:
                    print("{0}({1})".format(setobj['name'], setobj['code']))
                continue

            if name == '--getsetinfo':
                setshortname = value  # 'akh
                cardsinfo = getsetinfo(setshortname)
                for cardobj in cardsinfo:
                    try:
                        print("{0}\t{1}".format(
                            cardobj['name'], cardobj['mana_cost']))
                    except KeyError:
                        for face in cardobj['card_faces']:
                            print("{0}\t{1}".format(
                                face['name'], face['mana_cost']))
                continue

            if name == '--getcardinfo':
                cardinfo = getcardinfo_fromname(value)
                try:
                    print("{0}:\'{1}\'".format('card name', cardinfo['name']))
                    print("{0}:\'{1}\'".format(
                        'mana cost', cardinfo['mana_cost']))
                    print("{0}:\'{1}\'".format(
                        'card type', cardinfo['type_line']))
                    print("{0}:\n\'{1}\'".format(
                        'card text', cardinfo['oracle_text']))
                except KeyError:
                    for face in cardinfo['card_faces']:
                        print("{0}:\'{1}\'".format('card name', face['name']))
                        print("{0}:\'{1}\'".format(
                            'mana cost', face['mana_cost']))
                        print("{0}:\'{1}\'".format(
                            'card type', face['type_line']))
                        print("{0}:\n\'{1}\'".format(
                            'card text', face['oracle_text']))
                continue

            if name == '--downloadformat':
                formatname = value
                downloadformat(formatname)
                continue

            if name == '--downloadset':
                setshortname = value
                downloadset(setshortname)
                continue

            if name == '--downloadcube':
                setshortname = value
                downloadcube(setshortname)
                continue

            if name == '--downloaddeck':
                deckname = value
                downloaddeck(deckname)
                continue

            if name == '--downloadcard':
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
