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
    """Get series of information by represented setlongname"""
    try:
        cardinfo = []
        cardinfourl = 'http://gatherer.wizards.com/Pages/Search/'\
        'Default.aspx?sort=cn+&set=[\"{0}\"]'.format(setlongname)
        resp = requests.Session().get(cardinfourl, timeout=13, cookies={
            'CardDatabaseSettings': '0=1&1=28&2=0&14=1&3=13&4=0&5=1&6=15\
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
        print("\nTimeOutError:\n\tGet set {0} info time out".format(
            setlongname))
        exit(False)
    except (AttributeError, TypeError):
        print("\nSet:{0} info not find\n".format(setlongname), file=sys.stderr)
        exit(False)


def downloadimage(cardname, cardid):
    """Download the card image represented by cardname
    Cardid is used to determine the picture link"""
    renamecount = 0
    basecardname = cardname
    # a set base land number max value
    flag = 8
    try:
        cardurl = 'http://gatherer.wizards.com/Handlers/'\
        'Image.ashx?multiverseid={0}&type=card'.format(cardid)
        imageobject = requests.get(cardurl, timeout=13)
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        print("\nTimeOutError:\n\tDownload Card {0} request timeout stop downloading!\n".format(
              cardname), file=sys.stderr)
    try:
        if 'image' in imageobject.headers['Content-Type']:
            while flag:
                flag -= 1
                try:
                    # x mode in python3.3+
                    open(cardname + '.full.jpg', 'xb').write(imageobject.content)
                    print("Download card:{0} success,the card id is {1}".format
                          (cardname, cardid))
                    break
                except FileExistsError:
                    # rename base land
                    renamecount += 1
                    cardname = basecardname + str(renamecount)
        else:
            print("\nContent-Type Error:\n\trequest type not is image"\
            ",the card is:{0},number is:{1}\n".format(cardname, cardid), file=sys.stderr)
    except (AttributeError, TypeError, KeyError):
        print("\nThe card:{0} information obtained is wrong\n".format(
              cardname), file=sys.stderr)


def main():
    setshortname = input('You plan download setshortname:')
    setlongname = input('You plan download setlongname:')
    if os.path.exists('./' + setshortname) is False:
        os.mkdir('./' + setshortname)
    os.chdir('./' + setshortname)
    cardsinfo = getcardsinfo(setlongname)
    P = Pool(processes=4)
    print("Download set:{0} start,Card total {1}".format(setshortname, len(cardsinfo)))
    for cardobj in cardsinfo:
        P.apply_async(downloadimage, args=(cardobj[0], cardobj[1], ))
        #downloadimage( cardobj )
    P.close()
    P.join()
    print('Set {0} all card image download end'.format(setshortname))


if __name__ == '__main__':
    main()
