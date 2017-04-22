#!/usr/bin/python3
# coding=utf-8

import os
#import re
from multiprocessing import Pool

import requests
from bs4 import BeautifulSoup


def GetSetInfo( SetLongName ):
    CardInfo = []
    CardInfoUrl = 'http://gatherer.wizards.com/Pages/Search/Default.aspx?sort=cn+&set=[%%22%s%%22]' %SetLongName
    try:
        resp = requests.Session().get( CardInfoUrl , timeout = 13 , cookies={ 'CardDatabaseSettings':'0=1&1=zh-CN&2=0&14=1&3=13&4=0&5=1&6=15&7=0&8=1&9=1&10=18&11=7&12=8&15=1&16=0&13=' } )
    except (requests.exceptions.ReadTimeout,requests.exceptions.ConnectTimeout):
        print( "\nTimeOutError:\n\tGet set %s info time out" %SetLongName )
        exit( False )
    #cn cookies { 'CardDatabaseSettings': '0=1&1=zh-CN&2=0&14=1&3=13&4=0&5=1&6=15&7=0&8=1&9=1&10=18&11=7&12=8&15=1&16=0&13= ' }
    #en cookies { 'CardDatabaseSettings': '0=1&1=28&2=0&14=1&3=13&4=0&5=1&6=15&7=0&8=1&9=1&10=18&11=7&12=8&15=1&16=0&13= ' }                                             
    html = resp.text
    soup = BeautifulSoup( html , 'html.parser' )
    try:
        table = soup.find( 'table' , { 'class' : 'checklist' } )
        CardObjList = table.findAll( 'a' , { 'class' : 'nameLink' })
        for CardObj in CardObjList:
            CardID = int(CardObj['href'].partition('=')[2])
            CardName = CardObj.get_text()
            #re.sub( r'</?\w+[^>]*>' , '' , str( CardObj ) )
            CardInfo.append( ( CardName , CardID ) )
        return CardInfo
    except ( AttributeError , TypeError ):
        print( "Set %s info not find" %SetLongName )
        exit( False )

def Downlaod( CardObj ):
    CardUrl = 'http://gatherer.wizards.com/Handlers/Image.ashx?multiverseid=%d&type=card' %CardObj[1]
    try:
        imageobject = requests.get( CardUrl , timeout = 13 )
    except ( requests.exceptions.ReadTimeout , requests.exceptions.ConnectTimeout ):
        print( "\nTimeOutError:\n\tDownload Card %s request timeout stop downloading!" %CardObj[0] )
        exit( False )
    if imageobject.status_code == 200:
        open( CardObj[0] + '.full.jpg' , 'wb' ).write( imageobject.content )
        print( "Download card:%s success,the card id is %d" %( CardObj[0] , CardObj[1] ) )
    else:
        print( "\nContent-Type Error:\n\trequest not is jpeg image file,the card is %s number is:%d" %( CardObj[0] , CardObj[1] ) )

if __name__ == '__main__':
    SetShortName = input( 'You plan download setname(ShortName):' )
    SetLongName =  input( 'You plan download setname(LongName):' )
    if os.path.exists( './' + SetShortName ) == False:
        os.mkdir( './' + SetShortName )
    os.chdir( './' + SetShortName )
    CardInfo = GetSetInfo( SetLongName )
    p = Pool( processes = 4 )
    print( "Download start,Card total %d" %len( CardInfo ) )
    for CardObj in CardInfo:
        p.apply_async( Downlaod , args=( CardObj , ) )
        #Downlaod( CardObj )
    p.close()
    p.join()
    print( 'All download success' )
