#!/usr/bin/python3
# coding=utf-8

"""Get card image in http://magic.wizards.com/$(language)/articles/archive/card-image-gallery/"""

import os
import sys
from multiprocessing import Pool

import requests
from bs4 import BeautifulSoup

BASEURL = 'http://magic.wizards.com/{language}/articles/archive/card-image-gallery/{setlongname}'


def getcardsinfo(setlongname, language='zh-hans'):
    """Get series of information"""
    try:
        cardinfo = []
        # if card sequence is inconsistent function return error cardinfo
        addreurl = BASEURL.format(language=language, setlongname=setlongname)
        nameurl = BASEURL.format(language='en', setlongname=setlongname)
        resp_addre = requests.get(addreurl, timeout=13)
        html_addre = resp_addre.text
        soup_addre = BeautifulSoup(html_addre, 'html.parser')
        divs_addre = soup_addre.find_all('div', class_='resizing-cig')
        resp_name = requests.get(nameurl, timeout=13)
        html_name = resp_name.text
        soup_name = BeautifulSoup(html_name, 'html.parser')
        divs_name = soup_name.find_all('div', class_='resizing-cig')
        for div_name, divs_addre in zip(divs_name, divs_addre):
            img_name = div_name.find('img', alt=True)
            img_addre = divs_addre.find('img', alt=True)
            cardname = img_name['alt']
            cardurl = img_addre['src']
            cardname = cardname.replace('’', '\'')
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
            print("\nContent-Type Error:\n\trequest not is jpeg image file",
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
