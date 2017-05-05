#!/usr/bin/python3
# coding=utf-8

import os
import sys
import re
from multiprocessing import Pool

import requests
from bs4 import BeautifulSoup


def GetCardsInfo(SetShortName):
    try:
        CardInfo = []
        resp = requests.get('http://magiccards.info/' +
                            SetShortName + '/en.html', timeout=13)
        html = resp.text
        soup = BeautifulSoup(html, 'html.parser')
        table = soup.find('table', {'cellpadding': 3})
        for row in table.find_all('tr', class_=('odd', 'even')):
            NameObj = row.find('a')
            NumberObj = row.find('td', {'align': 'right'})
            name = NameObj.get_text()
            #name = re.sub( r'</?\w+[^>]*>' , '' , str( row.find( 'a' ) ) )
            number = NumberObj.get_text()
            #number = re.sub( r'</?\w+[^>]*>' , '' , str( row.find( 'td' , { 'align' : 'right' } ) ) )
            CardInfo.append((number, name))
        return CardInfo
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout, requests.exceptions.ConnectionError):
        print("\nTimeOutError:\n\tGet set:%s info time out" % SetShortName)
        exit(False)
    except (AttributeError, TypeError):
        print("\nSet %s info not find\n" % SetShortName, file=sys.stderr)
        exit(False)


def DownloadImage(SetShortName, CardID, CardName):
    try:
        ImageDownloadUrl = 'http://magiccards.info/scans/cn/' + \
            SetShortName + '/' + CardID + '.jpg'
        imageobject = requests.get(ImageDownloadUrl, timeout=13 )
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
    except (AttributeError, TypeError, KeyError):
        print("\nThe card:%s information obtained is wrong\n" %
              CardName, file=sys.stderr)
    except FileNotFoundError:
        p = re.compile(r'((\w*)/(\w*))')
        m = p.search(CardName)
        try:
            CookiesCardName = m.group(2) + m.group(3)
        except AttributeError:
            print( '\nUnknown format Card Name\n' , file=sys.stderr )
        open(CookiesCardName + '.full.jpg', 'wb').write(imageobject.content)
        print("Download cookiescard:%s success,the number is:%s" %
              (CardName, CardID))


if __name__ == '__main__':
    SetShortName = input('You plan download setshortname:')
    CardsInfo = GetCardsInfo(SetShortName)
    if os.path.exists('./' + SetShortName) is False:
        os.mkdir('./' + SetShortName)
    os.chdir('./' + SetShortName)
    p = Pool(processes=4)
    print("Download set:%s start,Card total %d" %( SetShortName , len(CardsInfo) ) )
    for CardObj in CardsInfo:
        p.apply_async(DownloadImage, args=(
            SetShortName, CardObj[0], CardObj[1], ))
        #DownloadImage( SetShortName , CardObj[0] , CardObj[1] )
    p.close()
    p.join()
    print('Set %s all card image download end' %SetShortName)
