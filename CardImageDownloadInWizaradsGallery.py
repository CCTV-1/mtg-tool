#!/usr/bin/python3
# coding=utf-8

"""Get card image in http://magic.wizards.com/$(language)/articles/archive/card-image-gallery/"""

import os
import sys
from multiprocessing import Pool

import requests
from bs4 import BeautifulSoup

BASEURL = 'http://magic.wizards.com/en/articles/archive/card-image-gallery/{setlongname}'


def getcardsinfo(setlongname, localcode='cs'):
    """localcode:en -> english,cs -> simplified chinese,ct -> traditional chinese,jp -> Japanese"""
    try:
        cardinfo = []
        # if card sequence is inconsistent function return error cardinfo
        url = BASEURL.format(setlongname=setlongname)
        resp = requests.get(url, timeout=13)
        html = resp.text
        soup = BeautifulSoup(html, 'html.parser')
        divs = soup.find_all('div', class_='resizing-cig')
        for div in divs:
            imgs = div.find_all('img', alt=True)
            # for meld card and double-face card,two card image in div
            for img in imgs:
                cardname = img['alt']
                cardurl = img['src']
                webname = cardurl.split('/')[-1]
                # http://***/en_***.png to http://***/localcode_***.png
                cardurl = cardurl.replace(
                    webname, webname.replace(webname[0:2], localcode))
                # fullwidth to halfwidth
                cardname = cardname.replace('’', '\'')
                # meld card back alt attribute value deal with
                cardname = cardname.replace(' (Bottom)', '')
                cardname = cardname.replace(' (Top)', '')
                cardinfo.append((cardname, cardurl))
        return cardinfo
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        print('\nTimeOutError:\n\tget set info time out')
        exit(False)
    except (AttributeError, TypeError, KeyError):
        print("\nSet info not find\n", file=sys.stderr)
        exit(False)


def downloadimage(cardname, cardurl):
    """Download the card image represented by cardname
    cardurl is used to determine the picture link"""
    renamecount = 0
    basecardname = cardname
    # a set base land number max value
    flag = 8
    try:
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
                    print("Download card:{0} success".format(cardname))
                    break
                except FileNotFoundError:
                    cardname = cardname.replace(' // ', '')
                    print("cookiescard:{0} rename to:{1} ".format(
                        basecardname, cardname))
                except FileExistsError:
                    # rename base land
                    renamecount += 1
                    cardname = '{0}{1}'.format(basecardname, renamecount)
        else:
            print("\nContent-Type Error:\n\t{0} request not is jpeg image file".format(cardname),
                  file=sys.stderr)
    except (AttributeError, TypeError, KeyError):
        print("\ncard:{0} info not error\n".format(cardname), file=sys.stderr)


def main():
    """main function"""
    setshortname = input('You plan download setshortname:')
    setlongname = input('You plan download setlongname:')
    # Aether Revolt to Aether-Revolt
    setlongname = setlongname.replace(' ', '-')
    if os.path.exists('./' + setshortname) is False:
        os.mkdir('./' + setshortname)
    os.chdir('./' + setshortname)
    cardsinfo = getcardsinfo(setlongname)
    processpool = Pool(processes=4)
    print("Download set:{0} start,Card total {1}".format(
        setshortname, len(cardsinfo)))
    for cardobj in cardsinfo:
        processpool.apply_async(
            downloadimage, args=(cardobj[0], cardobj[1], ))
    processpool.close()
    processpool.join()
    print('Set {0} all card image download end'.format(setshortname))


if __name__ == '__main__':
    main()
