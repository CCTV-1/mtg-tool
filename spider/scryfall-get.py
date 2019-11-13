#!/usr/bin/python3
# coding=utf-8

"""get card image in https://scryfall.com"""

import argparse
import logging
import os
import re
from concurrent.futures import ThreadPoolExecutor

import requests


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
    return list(set(cardnamelist))


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

        while has_more:
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


def getsetcards(setshortname, lang='en'):
    """get series information"""
    return get_queue_cardlist("s", setshortname)


def getcubecards(setshortname, lang='en'):
    """get cube information"""
    return get_queue_cardlist("cube", setshortname)


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
    with ThreadPoolExecutor(max_workers=8) as P:
        futures = {P.submit(downloadcard, cardobj)                   : cardobj for cardobj in cardlist}
        for future in futures:
            future.result()
    os.chdir('../')


def downloadformat(formatname, lang='en'):
    cardsinfo = getformatinfo(formatname, lang)
    donwload_cardlist(formatname, cardsinfo)


def downloadset(setname, lang='en'):
    cardsinfo = getsetcards(setname, lang)
    donwload_cardlist(setname, cardsinfo)


def downloadcube(cubename, lang='en'):
    cardsinfo = getcubecards(cubename, lang)
    donwload_cardlist(cubename, cardsinfo)


def downloaddeck(deckname, lang='en'):
    cardlist = getcardlist(deckname)
    cardsinfo = []
    with ThreadPoolExecutor(max_workers=8) as P:
        futures = {P.submit(getcardinfo_fromname, card)                   : card for card in cardlist}
        for future in futures:
            cardsinfo.append(future.result())

    donwload_cardlist("{0}_images".format(deckname), cardsinfo)


def downloadcard(cardobj, rename_flags=True, resolution='large', filename_format='xmage'):
    download_descptions = []
    if rename_flags:
        open_flags = 'xb'
    else:
        open_flags = 'wb'
    try:
        download_descptions.append(
            (cardobj['name'], cardobj['image_uris'][resolution]))
    except (KeyError):
        for card_face in cardobj['card_faces']:
            download_descptions.append(
                (card_face['name'], card_face['image_uris'][resolution]))

    for download_descption in download_descptions:
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
        except Exception as e:
            logging.info(
                "card:'%s',download failure.exception:'%s'", cardname, str(e))


def main():
    logging.basicConfig(filename='GetImage.log',
                        filemode='w', level=logging.INFO)
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--getallset", action="store_true", help="get scryfall.com supported setlist and each set shortname")
    group.add_argument("--downloadformat", metavar=("format name"), nargs=1, choices=["standard", "pioneer", "modern", "legacy", "vintage"],
                       help="download choice format all card image")
    group.add_argument("--downloadset", metavar=("set name"), nargs='+',
                       help="download set all card image")
    group.add_argument("--downloadcube", metavar=("cube name"), nargs='+',
                       help="download cube all card image")
    group.add_argument(
        "--downloaddeck", metavar=("deck name"), nargs='+', help="download deck content all card image")
    group.add_argument("--downloadcard", metavar=("card name"), nargs='+',
                       help="download card image,but not support download reprint card history image")
    args = parser.parse_args()

    if args.getallset:
        setlist = getsetlist()
        print('support set is:\n')
        for setobj in setlist:
            print("{0}({1})".format(setobj['name'], setobj['code']))

    if args.downloadformat:
        for formatname in args.downloadformat:
            downloadformat(formatname)

    if args.downloadset:
        for setname in args.downloadset:
            downloadset(setname)

    if args.downloadcube:
        for cubename in args.downloadcube:
            downloadcube(cubename)

    if args.downloaddeck:
        for deckname in args.downloaddeck:
            downloaddeck(deckname)

    if args.downloadcard:
        for cardname in args.downloadcard:
            cardinfo = getcardinfo_fromname(cardname)
            downloadcard(cardinfo, False)


if __name__ == '__main__':
    main()
