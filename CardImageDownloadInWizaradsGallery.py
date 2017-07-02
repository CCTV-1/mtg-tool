#!/usr/bin/python3
# coding=utf-8

"""Get card image in http://magic.wizards.com/en/articles/archive/card-image-gallery/"""

import os
import sys
from multiprocessing import Pool

import requests
from bs4 import BeautifulSoup


def getcardsinfo(addreurl, nameurl=None):
    """Get series of information"""
    try:
        cardinfo = []
        resp_addre = requests.get(addreurl, timeout=13)
        html_addre = resp_addre.text
        soup_addre = BeautifulSoup(html_addre, 'html.parser')
        divs_addre = soup_addre.find_all('div', class_='resizing-cig')
        # if card sequence is inconsistent function return error cardinfo
        if nameurl != None:
            resp_name = requests.get(nameurl, timeout=13)
            html_name = resp_name.text
            soup_name = BeautifulSoup(html_name, 'html.parser')
            divs_name = soup_name.find_all('div', class_='resizing-cig')
            for div_name, divs_addre in zip(divs_name, divs_addre):
                img_name = div_name.find('img', alt=True)
                img_addre = divs_addre.find('img', alt=True)
                cardname = img_name['alt']
                cardurl = img_addre['src']
                cardinfo.append((cardname, cardurl))
            return cardinfo
        else:
            for div in divs_addre:
                img = div.find('img', alt=True)
                cardname = img['alt']
                cardurl = img['src']
                cardinfo.append((cardname, cardurl))
        return cardinfo
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        print('\nTimeOutError:\n\tget set info time out')
        exit(False)
    except (AttributeError, TypeError):
        print("\nSet info not find\n", file=sys.stderr)
        exit(False)


def downloadimage(cardname, cardurl):
    """Download the card image represented by cardname
    cardurl is used to determine the picture link"""
    try:
        imageobject = requests.get(cardurl, timeout=13)
        if 'image' in imageobject.headers['Content-Type']:
            open(cardname + '.full.jpg', 'wb').write(imageobject.content)
            print("Download card:%s success" % cardname)
        else:
            print("\nContent-Type Error:\n\trequest not is jpeg image file" %
                  cardname, file=sys.stderr)
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        print("\nTimeOutError:\n\tDownload Card %s request timeout stop downloading!\n" %
              cardname, file=sys.stderr)
    except (AttributeError, TypeError, KeyError):
        print("\nThe card:%s information obtained is wrong\n" %
              cardname, file=sys.stderr)
    except FileNotFoundError:
        cookiescardname = cardname.replace(' // ', '')
        open(cookiescardname + '.full.jpg', 'wb').write(imageobject.content)
        print("Download cookiescard:%s success" % cardname)


def main():
    """main function"""
    setshortname = input('You plan download setshortname:')
    cards_name_infourl = input('You plan download card name info url:')
    cards_addre_infourl = input('You plan download card address url:')
    if os.path.exists('./' + setshortname) is False:
        os.mkdir('./' + setshortname)
    os.chdir('./' + setshortname)
    cardsinfo = getcardsinfo(cards_addre_infourl, cards_name_infourl)
    processpool = Pool(processes=4)
    print("Download set:%s start,Card total %d" %
          (setshortname, len(cardsinfo)))
    for cardobj in cardsinfo:
        processpool.apply_async(downloadimage, args=(cardobj[0], cardobj[1], ))
    processpool.close()
    processpool.join()
    print('Set %s all card image download end' % setshortname)


if __name__ == '__main__':
    main()
