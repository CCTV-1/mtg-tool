#!/usr/bin/python3
# coding=utf-8

"""Get card image in http://gatherer.wizards.com/Pages/Default.aspx"""

import logging
import os
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
        logging.info("Get set %s info time out", setlongname)
        exit(False)
    except (AttributeError, TypeError):
        logging.info("Set:%s info not find\n", setlongname)
        exit(False)


def downloadimage(cardname, cardid, filename_format='forge'):
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
        logging.info(
            "Download Card %s request timeout stop downloading!\n", cardname)
    try:
        if 'image' in imageobject.headers['Content-Type']:
            while flag:
                flag -= 1
                try:
                    # x mode in python3.3+
                    open(cardname + '.full.jpg', 'xb').write(imageobject.content)
                    logging.info(
                        "Download card:%s success,the card id is %s\n", cardname, cardid)
                    break
                except FileNotFoundError:
                    if filename_format == 'forge':
                        cardname = cardname.replace(' // ', '')
                    elif filename_format == 'xmage':
                        cardname = cardname.replace('//', '-')
                    else:
                        pass
                except FileExistsError:
                    # rename base land
                    renamecount += 1
                    cardname = basecardname + str(renamecount)
        else:
            logging.info("Request type not is image"
                         ",the card is:%s,number is:%s\n", cardname, cardid)
    except (AttributeError, TypeError, KeyError):
        logging.info("The card:%s information obtained is wrong\n", cardname)


def main():
    """main function"""
    logging.basicConfig(filename='GetImage.log',
                        filemode='w', level=logging.INFO)
    setshortname = input('You plan download setshortname:')
    setlongname = input('You plan download setlongname:')
    if os.path.exists('./' + setshortname) is False:
        os.mkdir('./' + setshortname)
    os.chdir('./' + setshortname)
    cardsinfo = getcardsinfo(setlongname)
    P = Pool(processes=4)
    print("Download set:{0} start,Card total {1}".format(
        setshortname, len(cardsinfo)))
    for cardobj in cardsinfo:
        P.apply_async(downloadimage, args=(cardobj[0], cardobj[1], ))
        #downloadimage( cardobj )
    P.close()
    P.join()
    print('Set {0} all card image download end'.format(setshortname))
    os.chdir('./..')


if __name__ == '__main__':
    main()
