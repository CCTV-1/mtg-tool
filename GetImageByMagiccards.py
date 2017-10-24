#!/usr/bin/python3
# coding=utf-8

"""Get series card image in https://magiccards.info/"""

import logging
import os
import re
from multiprocessing import Pool

import requests
from bs4 import BeautifulSoup


def getcardsinfo(setshortname):
    """Get Series of information by represented setshortname"""
    try:
        cardinfo = []
        resp = requests.get('https://magiccards.info/{0}/en.html'
                            .format(setshortname), timeout=13)
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
        logging.info("Get set:%s info time out", setshortname)
        exit(False)
    except (AttributeError, TypeError):
        logging.info("Set %s info not find\n", setshortname)
        exit(False)


def downloadimage(setshortname, cardid, cardname):
    """Download the card image represented by cardname
    cardid and setshortname is used to determine the picture link
    """
    renamecount = 0
    basecardname = cardname
    # a set base land number max value
    flag = 8
    try:
        imagedownloadurl = 'https://magiccards.info/scans/en/' + \
            setshortname + '/' + cardid + '.jpg'
        imageobject = requests.get(imagedownloadurl, timeout=13)
        if 'image' in imageobject.headers['Content-Type']:
            while flag:
                flag -= 1
                try:
                    # x mode in python3.3+
                    open(cardname + '.full.jpg', 'xb').write(imageobject.content)
                    logging.info("Download card:%s success,the number is:%s", cardname, cardid)
                    break
                except FileNotFoundError:
                    patternobject = re.compile(r'((\w*)/(\w*))')
                    matchobject = patternobject.search(cardname)
                    try:
                        cardname = matchobject.group(2) + matchobject.group(3)
                    except AttributeError:
                        logging.info('Unknown format Card Name\n')
                        logging.info("cookiescard:%s rename to:%s\n", basecardname, cardname)
                except FileExistsError:
                    # rename base land
                    renamecount += 1
                    cardname = '{0}{1}'.format(basecardname, renamecount)
        else:
            logging.info("Request type not is image\
            ,the card is:%s,number is:%s\n", cardname, cardid)
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        logging.info("Download Card %s request timeout stop downloading!\n", cardname)
    except (AttributeError, TypeError, KeyError):
        logging.info("The card:%s information obtained is wrong\n", cardname)


def main():
    """main function"""
    logging.basicConfig(filename='GetImage.log', filemode='w', level=logging.DEBUG)
    setshortname = input('You plan download setshortname:')
    cardinfo = getcardsinfo(setshortname)
    if os.path.exists('./' + setshortname) is False:
        os.mkdir('./' + setshortname)
    os.chdir('./' + setshortname)
    P = Pool(processes=4)
    logging.info("Download set:%s start,Card total %d", setshortname, len(cardinfo))
    for cardobj in cardinfo:
        P.apply_async(downloadimage, args=(
            setshortname, cardobj[0], cardobj[1], ))
        #downloadimage( setshortname , cardobj[0] , cardobj[1] )
    P.close()
    P.join()
    logging.info('Set %s all card image download end', setshortname)
    os.chdir('./..')

if __name__ == '__main__':
    main()
