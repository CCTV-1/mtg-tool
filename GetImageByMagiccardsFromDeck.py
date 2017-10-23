#!/usr/bin/env python3
# coding=utf-8

"""Get deck card image in https://magiccards.info/query?q=!{cardname}&v=card&s=cname"""

import os
import sys
import re
from multiprocessing import Pool

import requests
from bs4 import BeautifulSoup

def get_image_url(cardnamelist):
    """Get list card image url"""
    
    return ()

def get_cardinfo(filename):
    """Read deck file,get card info"""
    cardnamelist = []
    with open(filename, 'r') as deckfile:
        for line in deckfile:
            try:
                match = re.search(r'([0-9]+)\ ([^|]+)\|(.*)', line)
                cardname = match.group(2)
                cardurl = get_image_url(cardname)
                cardnamelist.append((cardname, cardurl))
            except IndexError:
                continue
    return cardnamelist

def downloadimage(cardname, cardurl):
    """Download card image"""
    try:
        imageobject = requests.get(cardurl, timeout=13)
        if 'image' in imageobject.headers['Content-Type']:
            # x mode in python3.3+
            open(cardname + '.full.jpg', 'xb').write(imageobject.content)
            print("Download card:{0} success,the url is:{1}".format
                  (cardname, cardurl))
        else:
            print("\nContent-Type Error:\n\trequest type not is image\
                ,the card is:{0},number is:{1}\n".format(cardname, cardurl), file=sys.stderr)
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        print("\nTimeOutError:\n\tDownload Card {0} request timeout stop downloading!\n".format(
            cardname), file=sys.stderr)
    except (AttributeError, TypeError, KeyError):
        print("\nThe card:{0} information obtained is wrong\n".format(
            cardname), file=sys.stderr)

def main():
    """main function"""
    deckfilename = input('You plan download card image deck file name:')
    cardinfo = get_cardinfo(deckfilename)
    if os.path.exists('./images') is False:
        os.mkdir('./images')
    os.chdir('./images')
    processes = Pool(processes=4)
    print("Download deck image start,Card total {0}".format
          (len(cardinfo)))
    for cardobj in cardinfo:
        processes.apply_async(downloadimage, args=(
            cardobj[0], cardobj[1], ))
        #downloadimage( setshortname , cardobj[0] , cardobj[1] )
    processes.close()
    processes.join()
    print('deck all card image download end')
    os.chdir('./..')

if __name__ == '__main__':
    main()
