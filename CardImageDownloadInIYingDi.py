#!/usr/bin/python3
# coding=utf-8

"""Get card image in http://www.iyingdi.com/web/tools/mtg/cards"""

import getopt
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
        print("\nTimeOutError:\n\tGet setlist time out", file=sys.stderr)
        exit(False)
    except (AttributeError, KeyError):
        print("\nNot get setlist!", file=sys.stderr)
        exit(False)


def getcardsinfo(seriesobj):
    """Get Series of information by represented setobj"""
    try:
        seriessize = seriesobj['size']
        seriesid = seriesobj['id']
        cardsinfo_tmp = []
        for page in range(math.ceil(seriessize / 20)):
            requestdata = {
                'order': '-seriesPubtime,+sindex',
                'series': str(seriesid),
                'size': '20',
                'page': str(page),
                #'statistic': 'total'
            }
            if page == 0:
                requestdata['statistic'] = 'total'
            resp = requests.post(
                'http://www.iyingdi.com/magic/card/search/vertical', data=requestdata, timeout=13)
            cardsinfoobj = resp.json()['data']['cards']
            for cardinfoobj in cardsinfoobj:
                cardsinfo_tmp.append(cardinfoobj)
        return cardsinfo_tmp
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        print("\nTimeOutError:\n\tGet set:%s card list time out" %
              setobj['ename'])
        exit(False)
    except (AttributeError, TypeError, KeyError):
        print("\nThe set information obtained is wrong\n", file=sys.stderr)


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
        print("\nTimeOutError:\n\tGet Card: %s info time out" % cardname)
        exit(False)
    except (AttributeError, TypeError, KeyError):
        print("\nGet Card:%s Info Failure\n" % cardname, file=sys.stderr)


def downloadimage(cardobj_parameters):
    """Download the card image represented by cardobj_parameters"""
    try:
        imagedownloadurl = cardobj_parameters['img']
        cardname = cardobj_parameters['ename']
        cardid = imagedownloadurl.split('/')[-1][0:-4]
        # url:******/$(cardid).jpg
        imageobject = requests.get(imagedownloadurl, timeout=13)
        if 'image' in imageobject.headers['Content-Type']:
            open(cardname + '.full.jpg', 'wb').write(imageobject.content)
            print("Download card:%s success,the number is:%s" %
                  (cardname, cardid))
        else:
            print("\nContent-Type Error:\n\trequest not is jpeg image file,\
            the card is %s number is:%s\n" % (cardname, cardid), file=sys.stderr)
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        print("\nTimeOutError:\n\tDownload Card %s request timeout stop downloading!\n" %
              cardname, file=sys.stderr)
    except (AttributeError, TypeError, KeyError):
        print("\nThe card:%s information obtained is wrong\n" %
              cardname, file=sys.stderr)
    except FileNotFoundError:
        cookiescardname = cardname.replace(' // ', '')
        open(cookiescardname + '.full.jpg', 'wb').write(imageobject.content)
        print("Download cookiescard:%s success,the number is:%s" %
              (cardname, cardid))


if __name__ == '__main__':
    try:
        OPTIONS, ARGS = getopt.getopt(sys.argv[1:], '', longopts=\
        ['help', 'getsetlist', 'getcardslist=', 'getcardinfo=', 'downloadset=', 'downloadcard='])
        for name, value in OPTIONS:
            if name in '--help':
                helps()
                continue

            if name in '--getsetlist':
                setlist = getsetlist()
                print('support set is:\n')
                for setobj in setlist:
                    print("%s(%s)" % (setobj['ename'], setobj['abbr']))
                continue

            if name in '--getcardslist':
                setlist = getsetlist()
                sethhortname = value  # 'akh
                for setobj in setlist:
                    if sethhortname in setobj['abbr']:
                        cardsinfo = getcardsinfo(setobj)
                        continue
                for cardobj in cardsinfo:
                    print('%s\t%s' % (cardobj['ename'], cardobj['mana']))
                continue

            if name in '--getcardinfo':
                CardInfo = getcardinfo(value)
                for InfoKey in CardInfo:
                    print("%s:%s" % (InfoKey, CardInfo[InfoKey]))
                continue

            if name in '--downloadset':
                setlist = getsetlist()
                sethhortname = value
                for setobj in setlist:
                    if sethhortname == setobj['abbr']:
                        CardsInfo = getcardsinfo(setobj)
                        setsize = len(CardsInfo)
                        break
                    else:
                        pass
                if os.path.exists('./' + sethhortname) is False:
                    os.mkdir('./' + sethhortname)
                os.chdir('./' + sethhortname)
                p = Pool(processes=4)
                print("Download set:%s start,Card total %d" %
                      (sethhortname, setsize))
                for cardobj in CardsInfo:
                    p.apply_async(downloadimage, args=(
                        cardobj, ))
                    # downloadimage(cardobj)
                p.close()
                p.join()
                print('Set %s all card image download end' % sethhortname)
                os.chdir('../')
                continue

            if name in '--downloadcard':
                cardobj = getcardinfo(value)
                downloadimage(cardobj)
                continue

    except getopt.GetoptError:
        helps()
