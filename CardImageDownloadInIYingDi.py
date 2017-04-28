#!/usr/bin/python3
# coding=utf-8

import getopt
import math
import os
import sys
from multiprocessing import Pool

import requests
from bs4 import BeautifulSoup


def helps():
    print(
        '--getsetlist get iyingdi supported setlist and each set shortname\n'
        '--getcardslist=[set shortname] get set cards list\n'
        '--downloadset=[set shortname] download set all card image\n'
        '--getcardinfo=[cardname] get card info(usage english name is best)\n'
        '--downloadcard=[cardname] download card image,but not support download reprint card history image'
    )


def GetSetList():
    try:
        resp = requests.get(
            'http://www.iyingdi.com/magic/series/list?size=-1', timeout=13)
        SetList = resp.json()['list']
        return SetList
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout, requests.exceptions.ConnectionError):
        print("\nTimeOutError:\n\tGet setlist time out", file=sys.stderr)
        exit(False)
    except (AttributeError, KeyError):
        print("\nNot get setlist!", file=sys.stderr)
        exit(False)


def GetCardsInfo(SetObj):
    try:
        SetSize = SetObj['size']
        SetID = SetObj['id']
        CardsInfo = []
        for page in range(math.ceil(SetSize / 20)):
            RequestData = {
                'order': '-seriesPubtime,+sindex',
                'series': str(SetID),
                'size': '20',
                'page': str(page),
                #'statistic': 'total'
            }
            if page == 0:
                RequestData['statistic'] = 'total'
            resp = requests.post(
                'http://www.iyingdi.com/magic/card/search/vertical', data=RequestData, timeout=13)
            CardsObj = resp.json()['data']['cards']
            for CardObj in CardsObj:
                CardsInfo.append(CardObj)
        return CardsInfo
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout, requests.exceptions.ConnectionError):
        print("\nTimeOutError:\n\tGet set:%s card list time out" %
              SetObj['ename'])
        exit(False)
    except (AttributeError, TypeError, KeyError):
        print("\nThe set information obtained is wrong\n", file=sys.stderr)


def GetCardInfo(CardName):
    try:
        RequestData = {
            'order': '-seriesPubtime,+sindex',
            'name': CardName,
            'size': '20',
            'page': '0',
            'statistic': 'total'
        }
        resp = requests.post(
            'http://www.iyingdi.com/magic/card/search/vertical', data=RequestData, timeout=13)
        CardInfo = resp.json()['data']['cards'][0]
        return CardInfo
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout, requests.exceptions.ConnectionError):
        print("\nTimeOutError:\n\tGet Card: %s info time out" % CardName)
        exit(False)
    except (AttributeError, TypeError, KeyError):
        print("\nGet Card:%s Info Failure\n" % CardName, file=sys.stderr)


def DownloadImage(CardObj):
    try:
        ImageDownloadUrl = CardObj['img']
        CardName = CardObj['ename']
        CardID = ImageDownloadUrl.split('/')[-1][0:-4]
        imageobject = requests.get(ImageDownloadUrl, timeout=13)
        if imageobject.headers['Content-Type'] == 'image/jpeg':
            open(CardName + '.full.jpg', 'wb').write(imageobject.content)
            print("Download card:%s success,the number is:%s" %
                  (CardName, CardID))
        else:
            print("\nContent-Type Error:\n\trequest not is jpeg image file,the card is %s number is:%s\n" %
                  (CardName, CardID), file=sys.stderr)
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout, requests.exceptions.ConnectionError):
        print("\nTimeOutError:\n\tDownload Card %s request timeout stop downloading!\n" %
              CardName, file=sys.stderr)
        exit(False)
    except (AttributeError, TypeError, KeyError):
        print("\nThe card:%s information obtained is wrong\n" %
              CardName, file=sys.stderr)
    except FileNotFoundError:
        CookiesCardName = CardName.replace(' // ', '')
        open(CookiesCardName + '.full.jpg', 'wb').write(imageobject.content)
        print("Download cookiescard:%s success,the number is:%s" %
              (CardName, CardID))


if __name__ == '__main__':
    try:
        options, args = getopt.getopt(sys.argv[1:], '',  longopts=[
                                      'help', 'getsetlist', 'getcardslist=', 'getcardinfo=', 'downloadset=', 'downloadcard='])
        for name, value in options:
            if name in '--help':
                helps()
                continue

            if name in '--getsetlist':
                SetList = GetSetList()
                print('support set is:\n')
                for SetObj in SetList:
                    print("%s(%s)" % (SetObj['ename'], SetObj['abbr']))
                continue

            if name in '--getcardslist':
                SetList = GetSetList()
                SetShortName = value  # 'akh
                for SetObj in SetList:
                    if SetShortName in SetObj['abbr']:
                        CardsInfo = GetCardsInfo(SetObj)
                        continue
                for CardObj in CardsInfo:
                    print('%s\t%s' % (CardObj['ename'], CardObj['mana']))
                continue

            if name in '--getcardinfo':
                CardInfo = GetCardInfo(value)
                for InfoKey in CardInfo:
                    print("%s:%s" % (InfoKey, CardInfo[InfoKey]))
                continue

            if name in '--downloadset':
                SetList = GetSetList()
                SetShortName = value  # 'akh
                for SetObj in SetList:
                    if SetShortName == SetObj['abbr']:
                        CardsInfo = GetCardsInfo(SetObj)
                        SetSize = len(CardsInfo)
                        break
                    else:
                        pass
                if os.path.exists('./' + SetShortName) == False:
                    os.mkdir('./' + SetShortName)
                os.chdir('./' + SetShortName)
                p = Pool(processes=4)
                print("Download set:%s start,Card total %d" %
                      (SetShortName, SetSize))
                for CardObj in CardsInfo:
                    p.apply_async(DownloadImage, args=(
                        CardObj, ))
                    # DownloadImage(CardObj)
                p.close()
                p.join()
                print('Set %s all card image download end' %SetShortName)
                os.chdir('../')
                continue

            if name in '--downloadcard':
                CardObj = GetCardInfo()
                DownloadImage(CardObj)
                continue

    except getopt.GetoptError:
        helps()
