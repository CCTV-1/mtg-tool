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
        print("\nTimeOutError:\n\tGet set:{0} card list time out".format(
              setobj['ename']))
        exit(False)
    except (AttributeError, TypeError, KeyError):
        print("\nThe set information obtained is wrong\n", file=sys.stderr)


def getcardinfo(cardname):
    """get card for information by represented cardname"""
    try:
        requestdata={
            'order': '-seriesPubtime,+sindex',
            'name': cardname,
            'size': '20',
            'page': '0',
            'statistic': 'total'
        }
        resp=requests.post(
            'http://www.iyingdi.com/magic/card/search/vertical', data=requestdata, timeout=13)
        cardinfo=resp.json()['data']['cards'][0]
        return cardinfo
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        print(
            "\nTimeOutError:\n\tGet Card: {0} info time out".format(cardname))
        exit(False)
    except (AttributeError, TypeError, KeyError):
        print("\nGet Card:{0} Info Failure\n".format(
            cardname), file=sys.stderr)


def downloadimage(cardobj_parameters):
    """Download the card image represented by cardobj_parameters"""
    renamecount=0
    # a set base land number max value
    flag=8
    try:
        basecardname=cardobj_parameters['ename']
        imagedownloadurl=cardobj_parameters['img']
        cardname=cardobj_parameters['ename']
        imageobject=requests.get(imagedownloadurl, timeout=13)
        if 'image' in imageobject.headers['Content-Type']:
            while flag:
                flag -= 1
                try:
                    # x mode in python3.3+
                    open(cardname + '.full.jpg', 'xb').write(imageobject.content)
                    print("Download card:{0} success".format(cardname))
                    break
                except FileNotFoundError:
                    cardname=cardname.replace(' // ', '')
                    print("cookiescard:{0} rename to:{1} ".format(
                        basecardname, cardname))
                except FileExistsError:
                    # rename base land
                    renamecount += 1
                    cardname=basecardname + str(renamecount)
        else:
            print("\nContent-Type Error:\n\trequest type not is image,\
            the card is:{0}\n".format(cardname), file=sys.stderr)
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        print("\nTimeOutError:\n\tDownload Card {0} request timeout stop downloading!\n".format(
              cardname), file=sys.stderr)
    except (AttributeError, TypeError, KeyError):
        print("\nThe card:{0} information obtained is wrong\n".format(
              cardname), file=sys.stderr)


def main():
    try:
        options, args=getopt.getopt(sys.argv[1:], '', longopts=[
                                    'help', 'getsetlist', 'getcardslist=', 'getcardinfo=', 'downloadset=', 'downloadcard='])
        for name, value in options:
            if name in '--help':
                helps()
                continue

            if name in '--getsetlist':
                setlist=getsetlist()
                print('support set is:\n')
                for setobj in setlist:
                    print("{0}({1})".format(setobj['ename'], setobj['abbr']))
                continue

            if name in '--getcardslist':
                setlist=getsetlist()
                setshortname=value  # 'akh
                for setobj in setlist:
                    if setshortname in setobj['abbr']:
                        cardsinfo=getcardsinfo(setobj)
                        continue
                for cardobj in cardsinfo:
                    print("{0}\t{1}".format(cardobj['ename'], cardobj['mana']))
                continue

            if name in '--getcardinfo':
                CardInfo=getcardinfo(value)
                for InfoKey in CardInfo:
                    print("{0}:{1}".format(InfoKey, CardInfo[InfoKey]))
                continue

            if name in '--downloadset':
                setlist=getsetlist()
                setshortname=value
                for setobj in setlist:
                    if setshortname == setobj['abbr']:
                        CardsInfo=getcardsinfo(setobj)
                        setsize=len(CardsInfo)
                        break
                    else:
                        pass
                if os.path.exists('./' + setshortname) is False:
                    os.mkdir('./' + setshortname)
                os.chdir('./' + setshortname)
                p=Pool(processes=4)
                print("Download set:{1} start,Card total {1}".format
                      (setshortname, setsize))
                for cardobj in CardsInfo:
                    p.apply_async(downloadimage, args=(
                        cardobj, ))
                    #downloadimage(cardobj)
                p.close()
                p.join()
                print('Set {0} all card image download end'.format(
                    setshortname))
                os.chdir('../')
                continue

            if name in '--downloadcard':
                cardobj=getcardinfo(value)
                downloadimage(cardobj)
                continue

    except getopt.GetoptError:
        helps()


if __name__ == '__main__':
    main()
