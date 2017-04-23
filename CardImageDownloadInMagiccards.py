#!/usr/bin/python3
# coding=utf-8

import os
import sys
#import re
from multiprocessing import Pool

import requests
from bs4 import BeautifulSoup


def GetSetInfo(SetShortName):
    CardInfo = []
    try:
        resp = requests.get('http://magiccards.info/' +
                            SetShortName + '/en.html', timeout=13)
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        print("\nTimeOutError:\n\tGet set %s info time out" % SetShortName)
        exit(False)
    html = resp.text
    soup = BeautifulSoup(html, 'html.parser')
    try:
        table = soup.find('table', {'cellpadding': 3})
        for row in table.find_all('tr', class_=('odd', 'even')):
            NameObj = row.find('a')
            NumberObj = row.find('td', {'align': 'right'})
            name = NameObj.get_text()
            #name = re.sub( r'</?\w+[^>]*>' , '' , str( row.find( 'a' ) ) )
            number = NumberObj.get_text()
            #number = re.sub( r'</?\w+[^>]*>' , '' , str( row.find( 'td' , { 'align' : 'right' } ) ) )
            CardInfo.append((number, name))
        return CardInfo
    except (AttributeError, TypeError):
        print("\n]rSet %s info not find]n" % SetShortName, file=sys.stderr)
        exit(False)


def DownloadImage(SetShortName, CardID, CardName):
    ImageDownloadUrl = 'http://magiccards.info/scans/cn/' + \
        SetShortName + '/' + CardID + '.jpg'
    try:
        imageobject = requests.get(ImageDownloadUrl, timeout=13)
    except (requests.exceptions.ReadTimeout, requests.exceptions.ConnectTimeout):
        print("\nTimeOutError:\n\tDownload Card %s request timeout stop downloading!\n" %
              CardName, file=sys.stderr)
        exit(False)
    if imageobject.headers['Content-Type'] == 'image/jpeg':
        open(CardName + '.full.jpg', 'wb').write(imageobject.content)
        print("Download card:%s success,the number is:%s" % (CardName, CardID))
    else:
        print("\nContent-Type Error:\n\trequest not is jpeg image file,the card is %s number is:%s\n" %
              (CardName, CardID), file=sys.stderr)


if __name__ == '__main__':
    SetShortName = input('You plan download setshortname:')
    CardInfo = GetSetInfo(SetShortName)
    if os.path.exists('./' + SetShortName) == False:
        os.mkdir('./' + SetShortName)
    os.chdir('./' + SetShortName)
    p = Pool(processes=4)
    print("Download start,Card total %d" % len(CardInfo)+1)
    for CardObj in CardInfo:
        p.apply_async(DownloadImage, args=(
            SetShortName, CardObj[0], CardObj[1], ))
        #DownloadImage( SetShortName , CardObj[0] , CardObj[1] )
    p.close()
    p.join()
    print('All download success')
