#!/usr/bin/python3
# coding=utf-8

import math
import os
import sys
from multiprocessing import Pool

import requests
from bs4 import BeautifulSoup


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
        print("\nTimeOutError:\n\tGet set %s info time out" % SetShortName)
        exit(False)
    except (AttributeError, TypeError, KeyError):
        print("\nThe set information obtained is wrong\n", file=sys.stderr)


def DownloadImage(SetShortName, CardObj):
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
    SetList = GetSetList()
    SetShortName = input('You plan download setshortname:')  # 'akh'
    for SetObj in SetList:
        if SetShortName in SetObj['abbr']:
            CardsInfo = GetCardsInfo(SetObj)
            SetSize = len(CardsInfo)
            break
        else:
            pass
    if os.path.exists('./' + SetShortName) == False:
        os.mkdir('./' + SetShortName)
    os.chdir('./' + SetShortName)
    p = Pool(processes=4)
    print("Download start,Card total %d" % SetSize)
    for CardObj in CardsInfo:
        p.apply_async(DownloadImage, args=(
            SetShortName, CardObj, ))
        #DownloadImage(SetShortName, CardObj)
    p.close()
    p.join()
    print('All download success')
