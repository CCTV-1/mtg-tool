#!/usr/bin/env python3
# coding=utf-8

"""Get deck card image in https://magiccards.info/
image info in https://magiccards.info/query?q=!{cardname}&v=card&s=cname"""

import logging
import os
import re
from multiprocessing import Pool

import requests
from bs4 import BeautifulSoup


def get_image_url(cardname):
    """Get card image url"""
    cardurl = None
    try:
        resp = requests.get('https://magiccards.info/query?q=!{0}&v=card&s=cname'
                            .format(cardname), timeout=13)
        html = resp.text
        soup = BeautifulSoup(html, 'html.parser')
        image = soup.find('img', {'style': 'border: 1px solid black;'})
        cardurl = "https://magiccards.info{0}".format(image['src'])

    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        logging.info("Get card:%s url time out", cardname)
    except (AttributeError, TypeError):
        logging.info("Card %s url not find\n", cardname)
    return cardurl

def get_cardinfo(filename):
    """Read deck file,get card info"""
    cardinfo = []
    cardnamelist = []
    with open(filename, 'r') as deckfile:
        for line in deckfile:
            try:
                match = re.search(r'([0-9]+)\ ([^|]+)\|(.*)', line)
                cardname = match.group(2)
                cardnamelist.append(cardname)
            except (IndexError, AttributeError):
                continue
    processes = Pool(processes=4)
    cardurllist = processes.map(get_image_url, cardnamelist)
    for cardname, cardurl in zip(cardnamelist, cardurllist):
        cardinfo.append((cardname, cardurl))
    return cardinfo

def downloadimage(cardname, cardurl):
    """Download card image"""
    try:
        imageobject = requests.get(cardurl, timeout=13)
        if 'image' in imageobject.headers['Content-Type']:
            # x mode in python3.3+
            open(cardname + '.full.jpg', 'xb').write(imageobject.content)
            logging.info("Download card:%s success,the url is:%s", cardname, cardurl)
        else:
            logging.info("Request type not is image,the card is:\
                %s,number is:%s\n", cardname, cardurl)
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        logging.info("Download Card %s request timeout stop downloading!\n", cardname)
    except (AttributeError, TypeError, KeyError):
        logging.info("The card:%s information obtained is wrong\n", cardname)

def main():
    """main function"""
    logging.basicConfig(filename='GetImage.log', filemode='w', level=logging.INFO)
    deckfilename = input('You plan download card image deck file name:')
    print('Get deck card information start')
    cardinfo = get_cardinfo(deckfilename)
    print('Get deck card information end')
    if os.path.exists('./images') is False:
        os.mkdir('./images')
    os.chdir('./images')
    processes = Pool(processes=4)
    print("Download deck image start,Card total {0}".format(len(cardinfo)))
    for cardobj in cardinfo:
        processes.apply_async(downloadimage, args=(
            cardobj[0], cardobj[1], ))
        #downloadimage( setshortname , cardobj[0] , cardobj[1] )
    processes.close()
    processes.join()
    print('Deck all card image download end')
    os.chdir('./..')

if __name__ == '__main__':
    main()
