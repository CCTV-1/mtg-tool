#!/usr/bin/python3
# coding=utf-8

"""Get card image in http://www.iyingdi.com/web/tools/mtg/cards"""

import getopt
import logging
import math
import os
import sys
from multiprocessing import Pool

import requests


def helps():
    """get help information"""
    print(
        '--getsetlist get iyingdi supported setlist and each set shortname\n'
        '--getcardslist=[set shortname] get set cards list\n'
        '--downloadset=[set shortname] download set all card image\n'
        '--getcardinfo=[cardname] get card info(usage english name is best)\n'
        '--downloadcard=[cardname] download card image\
        ,but not support download reprint card history image'
    )


def getsetlist():
    """get iyingdi supported Series list and each set shortname"""
    try:
        resp = requests.get(
            'http://www.iyingdi.com/magic/series/list?size=-1', timeout=13)
        serieslist = resp.json()['list']
        return serieslist
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        logging.info('Get setlist time out\n')
        exit(False)
    except (AttributeError, KeyError):
        logging.info('Not get setlist\n')
        exit(False)


def getcardsinfo(seriesobj):
    """Get Series of information by represented setobj"""
    try:
        seriesid = seriesobj['id']
        cardsinfo_tmp = []
        page = 0
        requestdata = {
            'order': '-seriesPubtime,+sindex',
            'series': str(seriesid),
            'size': '20',
            'page': str(page),
            'statistic': 'total'
        }
        resp = requests.post(
            'http://www.iyingdi.com/magic/card/search/vertical', data=requestdata, timeout=13)
        seriessize = resp.json()['data']['total']
        cardsinfoobj = resp.json()['data']['cards']
        responsesize = len(cardsinfoobj)
        for i in range(responsesize):
            cardsinfo_tmp.append(cardsinfoobj[i])
        for page in range(1, math.ceil(seriessize/responsesize)):
            requestdata = {
                'order': '-seriesPubtime,+sindex',
                'series': str(seriesid),
                'size': '20',
                'page': str(page),
            }
            resp = requests.post(
                'http://www.iyingdi.com/magic/card/search/vertical', data=requestdata, timeout=13)
            cardsinfoobj = resp.json()['data']['cards']
            responsesize = len(cardsinfoobj)
            for i in range(responsesize):
                cardsinfo_tmp.append(cardsinfoobj[i])
        return cardsinfo_tmp
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        logging.info("Get set:%s card list time out", seriesobj['ename'])
        exit(False)
    except (AttributeError, TypeError, KeyError):
        logging.info('Set:%s information obtained is wrong\n',
                     seriesobj['ename'])


def getcardinfo(cardname):
    """get card for information by represented cardname"""
    try:
        requestdata = {
            'order': '-seriesPubtime,+sindex',
            'name': cardname,
            'size': '20',
            'page': '0',
            'statistic': 'total'
        }
        resp = requests.post(
            'http://www.iyingdi.com/magic/card/search/vertical', data=requestdata, timeout=13)
        cardinfo = resp.json()['data']['cards'][0]
        return cardinfo
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        logging.info("Get Card: %s info time out", cardname)
        exit(False)
    except (AttributeError, TypeError, KeyError):
        logging.info("Get Card:%s Info Failure\n", cardname)


def downloadimage(cardobj_parameters, filename_format='xmage'):
    """Download the card image represented by cardobj_parameters"""
    renamecount = 0
    # a set base land number max value
    flag = 8
    try:
        basecardname = cardobj_parameters['ename']
        imagedownloadurl = cardobj_parameters['img']
        cardname = cardobj_parameters['ename']
        imageobject = requests.get(imagedownloadurl, timeout=13)
        if 'image' in imageobject.headers['Content-Type'] or 'application' in imageobject.headers['Content-Type']:
            while flag:
                flag -= 1
                try:
                    # x mode in python3.3+
                    open(cardname + '.full.jpg', 'xb').write(imageobject.content)
                    logging.info("Download card:%s success", cardname)
                    break
                except FileNotFoundError:
                    if filename_format == 'forge':
                        cardname = cardname.replace(' // ', '')
                    elif filename_format == 'xmage':
                        cardname = cardname.replace('//', '-')
                    else:
                        pass
                    logging.info("Cookiescard:%s rename to:%s\n",
                                 basecardname, cardname)
                except FileExistsError:
                    # rename base land
                    renamecount += 1
                    cardname = basecardname + str(renamecount)
        else:
            logging.info("Request type not is image,\
            the card is:%s\n", cardname)
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        logging.info(
            "Download Card %s request timeout stop downloading!\n", cardname)
    except (AttributeError, TypeError, KeyError):
        logging.info("The card:%s information obtained is wrong\n", cardname)


def main():
    """main function"""
    logging.basicConfig(filename='GetImage.log',
                        filemode='w', level=logging.INFO)
    try:
        options, args = getopt.getopt(sys.argv[1:], '', longopts=[
            'help', 'getsetlist', 'getcardslist=', 'getcardinfo=', 'downloadset=', 'downloadcard='])
        args = args  # wipe off unused warning
        for name, value in options:
            if name in '--help':
                helps()
                continue

            if name in '--getsetlist':
                setlist = getsetlist()
                print('support set is:\n')
                for setobj in setlist:
                    print("{0}({1})".format(setobj['ename'], setobj['abbr']))
                continue

            if name in '--getcardslist':
                setlist = getsetlist()
                setshortname = value  # 'akh
                for setobj in setlist:
                    if setshortname in setobj['abbr']:
                        cardsinfo = getcardsinfo(setobj)
                        continue
                for cardobj in cardsinfo:
                    print("{0}\t{1}".format(cardobj['ename'], cardobj['mana']))
                continue

            if name in '--getcardinfo':
                cardinfo = getcardinfo(value)
                for infokey in cardinfo:
                    print("{0}:{1}".format(infokey, cardinfo[infokey]))
                continue

            if name in '--downloadset':
                setlist = getsetlist()
                setshortname = value
                for setobj in setlist:
                    if setshortname == setobj['abbr']:
                        cardsinfo = getcardsinfo(setobj)
                        setsize = len(cardsinfo)
                        break
                    else:
                        pass
                if os.path.exists('./' + setshortname) is False:
                    os.mkdir('./' + setshortname)
                os.chdir('./' + setshortname)
                P = Pool(processes=4)
                print("Download set:{0} start,Card total {1}".format(
                    setshortname, setsize))
                for cardobj in cardsinfo:
                    P.apply_async(downloadimage, args=(
                        cardobj, ))
                    # downloadimage(cardobj)
                P.close()
                P.join()
                print('Set {0} all card image download end'.format(
                    setshortname))
                os.chdir('../')
                continue

            if name in '--downloadcard':
                cardobj = getcardinfo(value)
                downloadimage(cardobj)
                continue

    except getopt.GetoptError:
        helps()


if __name__ == '__main__':
    main()
