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
        print("\nTimeOutError:\n\tGet set:{0} info time out".format(
            setshortname))
        exit(False)
    except (AttributeError, TypeError):
        print("\nSet {0} info not find\n".format(
            setshortname), file=sys.stderr)
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
        imagedownloadurl = 'http://magiccards.info/scans/cn/' + \
            setshortname + '/' + cardid + '.jpg'
        imageobject = requests.get(imagedownloadurl, timeout=13)
        if 'image' in imageobject.headers['Content-Type']:
            while flag:
                flag -= 1
                try:
                    open(cardname + '.full.jpg', 'wb').write(imageobject.content)
                    print("Download card:{0} success,the number is:{1}".format
                          (cardname, cardid))
                    break
                except FileNotFoundError:
                    patternobject = re.compile(r'((\w*)/(\w*))')
                    matchobject = patternobject.search(cardname)
                    try:
                        cardname = matchobject.group(2) + matchobject.group(3)
                    except AttributeError:
                        print('\nUnknown format Card Name\n', file=sys.stderr)
                        print("cookiescard:{0} rename to:{1} ".format(
                            basecardname, cardname))
                except FileExistsError:
                    # rename base land
                    renamecount += 1
                    cardname = '{0}{1}'.format(basecardname, renamecount)
        else:
            print("\nContent-Type Error:\n\trequest not is jpeg image file\
            ,the card is {0} number is:{1}\n".format(cardname, cardid), file=sys.stderr)
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        print("\nTimeOutError:\n\tDownload Card {0} request timeout stop downloading!\n".format(
              cardname), file=sys.stderr)
    except (AttributeError, TypeError, KeyError):
        print("\nThe card:{0} information obtained is wrong\n".format(
              cardname), file=sys.stderr)


def main():
    setshortname = input('You plan download setshortname:')
    cardinfo = getcardsinfo(setshortname)
    if os.path.exists('./' + setshortname) is False:
        os.mkdir('./' + setshortname)
    os.chdir('./' + setshortname)
    P = Pool(processes=4)
    print("Download set:{0} start,Card total {1}".format
          (setshortname, len(cardinfo)))
    for cardobj in cardinfo:
        P.apply_async(downloadimage, args=(
            setshortname, cardobj[0], cardobj[1], ))
        #downloadimage( setshortname , cardobj[0] , cardobj[1] )
    P.close()
    P.join()
    print('Set {0} all card image download end'.format(setshortname))


if __name__ == '__main__':
    main()
