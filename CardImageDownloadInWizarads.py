#!/usr/bin/python3
# coding=utf-8

import os
import sys
#import re
from multiprocessing import Pool

import requests
from bs4 import BeautifulSoup


def GetCardsInfo(SetLongName):
    try:
        CardInfo = []
        CardInfoUrl = 'http://gatherer.wizards.com/Pages/Search/Default.aspx?sort=cn+&set=[%%22%s%%22]' % SetLongName
        resp = requests.Session().get(CardInfoUrl, timeout=13, cookies={
            'CardDatabaseSettings': '0=1&1=zh-CN&2=0&14=1&3=13&4=0&5=1&6=15&7=0&8=1&9=1&10=18&11=7&12=8&15=1&16=0&13='})
        # cn cookies { 'CardDatabaseSettings': '0=1&1=zh-CN&2=0&14=1&3=13&4=0&5=1&6=15&7=0&8=1&9=1&10=18&11=7&12=8&15=1&16=0&13= ' }
        # en cookies { 'CardDatabaseSettings':
        # '0=1&1=28&2=0&14=1&3=13&4=0&5=1&6=15&7=0&8=1&9=1&10=18&11=7&12=8&15=1&16=0&13=
        # ' }
        html = resp.text
        soup = BeautifulSoup(html, 'html.parser')
        table = soup.find('table', class_='checklist')
        CardObjList = table.find_all('a', class_='nameLink')
        for CardObj in CardObjList:
            CardID = int(CardObj['href'].partition('=')[2])
            CardName = CardObj.get_text()
            #re.sub( r'</?\w+[^>]*>' , '' , str( CardObj ) )
            CardInfo.append((CardName, CardID))
        return CardInfo
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout, requests.exceptions.ConnectionError):
        print("\nTimeOutError:\n\tGet set %s info time out" % SetLongName)
        exit(False)
    except (AttributeError, TypeError):
        print("\nSet:%s info not find\n" % SetLongName, file=sys.stderr)
        exit(False)


def Downlaod(CardName, CardID):
    try:
        CardUrl = 'http://gatherer.wizards.com/Handlers/Image.ashx?multiverseid=%d&type=card' % CardID
        imageobject = requests.get(CardUrl, timeout=13)
        if imageobject.status_code == 200:
            open(CardName + '.full.jpg', 'wb').write(imageobject.content)
            print("Download card:%s success,the card id is %d" %
                  (CardName, CardID))
        else:
            print("\nContent-Type Error:\n\trequest not is jpeg image file,the card is %s number is:%d\n" %
                  (CardName, CardID), file=sys.stderr)
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout, requests.exceptions.ConnectionError):
        print("\nTimeOutError:\n\tDownload Card %s request timeout stop downloading!\n" %
              CardName, file=sys.stderr)
        exit(False)
    except (AttributeError, TypeError, KeyError):
        print("\nThe card:%s information obtained is wrong\n" %
              CardName, file=sys.stderr)

if __name__ == '__main__':
    SetShortName = input('You plan download setshortname:')
    SetLongName = input('You plan download setlongname:')
    if os.path.exists('./' + SetShortName) == False:
        os.mkdir('./' + SetShortName)
    os.chdir('./' + SetShortName)
    CardsInfo = GetCardsInfo(SetLongName)
    p = Pool(processes=4)
    print("Download set:%s start,Card total %d" (% SetshortName , len(CardsInfo) ) )
    for CardObj in CardsInfo:
        p.apply_async(Downlaod, args=(CardObj[0], CardObj[1], ))
        #Downlaod( CardObj )
    p.close()
    p.join()
    print('Set %s all card image download end' %SetShortName)
