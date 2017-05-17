#!/usr/bin/python3
# coding=utf-8

"""Get card image in http://magiccards.info/"""

import os
import sys
import re
from multiprocessing import Pool

import requests
from bs4 import BeautifulSoup


def getcardsinfo(setshortname):
    """Get Series of information by represented setshortname"""
    try:
        cardinfo = []
        resp = requests.get('http://magiccards.info/' +
                            setshortname + '/en.html', timeout=13)
        html = resp.text
        soup = BeautifulSoup(html, 'html.parser')
        table = soup.find('table', {'cellpadding': 3})
        for row in table.find_all('tr', class_=('odd', 'even')):
            nameobj = row.find('a')
            numberobj = row.find('td', {'align': 'right'})
            name = nameobj.get_text()
            #name = re.sub( r'</?\w+[^>]*>' , '' , str( row.find( 'a' ) ) )
            number = numberobj.get_text()
            # number = re.sub( r'</?\w+[^>]*>' , '' , \
            # str( row.find( 'td' , { 'align' : 'right' } ) ) )
            cardinfo.append((number, name))
        return cardinfo
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        print("\nTimeOutError:\n\tGet set:%s info time out" % setshortname)
        exit(False)
    except (AttributeError, TypeError):
        print("\nSet %s info not find\n" % setshortname, file=sys.stderr)
        exit(False)


def downloadimage(setshortname, cardid, cardname):
    """Download the card image represented by cardname
    cardid and setshortname is used to determine the picture link
    """
    try:
        imagedownloadurl = 'http://magiccards.info/scans/cn/' + \
            setshortname + '/' + cardid + '.jpg'
        imageobject = requests.get(imagedownloadurl, timeout=13)
        if 'image' in imageobject.headers['Content-Type']:
            open(cardname + '.full.jpg', 'wb').write(imageobject.content)
            print("Download card:%s success,the number is:%s" %
                  (cardname, cardid))
        else:
            print("\nContent-Type Error:\n\trequest not is jpeg image file\
            ,the card is %s number is:%s\n" % (cardname, cardid), file=sys.stderr)
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        print("\nTimeOutError:\n\tDownload Card %s request timeout stop downloading!\n" %
              cardname, file=sys.stderr)
    except (AttributeError, TypeError, KeyError):
        print("\nThe card:%s information obtained is wrong\n" %
              cardname, file=sys.stderr)
    except FileNotFoundError:
        patternobject = re.compile(r'((\w*)/(\w*))')
        matchobject = patternobject.search(cardname)
        try:
            cookiesscardname = matchobject.group(2) + matchobject.group(3)
        except AttributeError:
            print('\nUnknown format Card Name\n', file=sys.stderr)
        open(cookiesscardname + '.full.jpg', 'wb').write(imageobject.content)
        print("Download cookiescard:%s success,the number is:%s" %
              (cardname, cardid))


if __name__ == '__main__':
    SETSHORTNAME = input('You plan download setshortname:')
    CARDINFO = getcardsinfo(SETSHORTNAME)
    if os.path.exists('./' + SETSHORTNAME) is False:
        os.mkdir('./' + SETSHORTNAME)
    os.chdir('./' + SETSHORTNAME)
    P = Pool(processes=4)
    print("Download set:%s start,Card total %d" %
          (SETSHORTNAME, len(CARDINFO)))
    for cardobj in CARDINFO:
        P.apply_async(downloadimage, args=(
            SETSHORTNAME, cardobj[0], cardobj[1], ))
        #downloadimage( SETSHORTNAME , cardobj[0] , cardobj[1] )
    P.close()
    P.join()
    print('Set %s all card image download end' % SETSHORTNAME)
