#!/usr/bin/python3
# coding=utf-8

"""Get card image in http://gatherer.wizards.com/Pages/Default.aspx"""

import os
import sys
#import re
from multiprocessing import Pool

import requests
from bs4 import BeautifulSoup


def getcardsinfo(setlongname):
    """Get Series of information by represented setlongname"""
    try:
        cardinfo = []
        cardinfourl = 'http://gatherer.wizards.com/Pages/Search/\
        Default.aspx?sort=cn+&set=[%%22%s%%22]' % setlongname
        resp = requests.Session().get(cardinfourl, timeout=13, cookies={
            'CardDatabaseSettings': '0=1&1=zh-CN&2=0&14=1&3=13&4=0&5=1&6=15\
            &7=0&8=1&9=1&10=18&11=7&12=8&15=1&16=0&13='})
        # cn cookies { 'CardDatabaseSettings': \
        # '0=1&1=zh-CN&2=0&14=1&3=13&4=0&5=1&6=15&7=0&8=1&9=1&10=18&11=7&12=8&15=1&16=0&13= ' }
        # en cookies { 'CardDatabaseSettings':\
        # '0=1&1=28&2=0&14=1&3=13&4=0&5=1&6=15&7=0&8=1&9=1&10=18&11=7&12=8&15=1&16=0&13= ' }
        html = resp.text
        soup = BeautifulSoup(html, 'html.parser')
        table = soup.find('table', class_='checklist')
        cardinfolist = table.find_all('a', class_='nameLink')
        for cardinfoobj in cardinfolist:
            cardid = int(cardinfoobj['href'].partition('=')[2])
            cardname = cardinfoobj.get_text()
            #re.sub( r'</?\w+[^>]*>' , '' , str( CardObj ) )
            cardinfo.append((cardname, cardid))
        return cardinfo
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        print("\nTimeOutError:\n\tGet set %s info time out" % setlongname)
        exit(False)
    except (AttributeError, TypeError):
        print("\nSet:%s info not find\n" % setlongname, file=sys.stderr)
        exit(False)


def downloadimage(cardname, cardid):
    """Download the card image represented by cardname
    Cardid is used to determine the picture link"""
    try:
        cardurl = 'http://gatherer.wizards.com/Handlers/\
        Image.ashx?multiverseid=%d&type=card' % cardid
        imageobject = requests.get(cardurl, timeout=13)
        if 'image' in imageobject.headers['Content-Type']:
            open(cardname + '.full.jpg', 'wb').write(imageobject.content)
            print("Download card:%s success,the card id is %d" %
                  (cardname, cardid))
        else:
            print("\nContent-Type Error:\n\trequest not is jpeg image file\
            ,the card is %s number is:%d\n" % (cardname, cardid), file=sys.stderr)
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        print("\nTimeOutError:\n\tDownload Card %s request timeout stop downloading!\n" %
              cardname, file=sys.stderr)
    except (AttributeError, TypeError, KeyError):
        print("\nThe card:%s information obtained is wrong\n" %
              cardname, file=sys.stderr)


if __name__ == '__main__':
    SETSHORINAME = input('You plan download setshortname:')
    SETLONGNAME = input('You plan download setlongname:')
    if os.path.exists('./' + SETSHORINAME) is False:
        os.mkdir('./' + SETSHORINAME)
    os.chdir('./' + SETSHORINAME)
    CARDSINFO = getcardsinfo(SETLONGNAME)
    P = Pool(processes=4)
    print("Download set:%s start,Card total %d" %
          (SETSHORINAME, len(CARDSINFO)))
    for cardobj in CARDSINFO:
        P.apply_async(downloadimage, args=(cardobj[0], cardobj[1], ))
        #downloadimage( cardobj )
    P.close()
    P.join()
    print('Set %s all card image download end' % SETSHORINAME)
