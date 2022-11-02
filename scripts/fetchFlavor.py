#!/usr/bin/python3
# coding=utf-8

"""Get card image in http://gatherer.wizards.com/Pages/Default.aspx"""

import json
import asyncio
import time

from bs4 import BeautifulSoup
import aiohttp

TSInfo: dict[int:dict] = {}


async def getCardsInfo(setlongname: str, userCookies: dict) -> dict[str:str]:
    cardinfo = {}
    cardinfourl = 'http://gatherer.wizards.com/Pages/Search/'\
        'Default.aspx?sort=cn+&set=[\"{0}\"]'.format(setlongname)

    async with aiohttp.ClientSession(cookies=userCookies) as session:
        async with session.get(cardinfourl) as response:
            html = await response.text()

    soup = BeautifulSoup(html, 'html.parser')
    table = soup.find('table', class_='checklist')
    cardinfolist = table.find_all('tr', class_='cardItem')
    for cardinfoobj in cardinfolist:
        collectorNumber = int(cardinfoobj.contents[0].contents[0])
        cardname = cardinfoobj.contents[1].get_text()
        cardid = int(cardinfoobj.contents[1].find(
            'a')['href'].partition('=')[2])

        if cardinfo.__contains__(cardname):
            if collectorNumber < cardinfo[cardname]['collectorNumber']:
                cardinfo[cardname] = {
                    'collectorNumber': collectorNumber, 'cardid': cardid}
        else:
            cardinfo[cardname] = {
                'collectorNumber': collectorNumber, 'cardid': cardid}
    return cardinfo


async def getCardFlavorText(cardid: int) -> str:
    async with aiohttp.ClientSession() as session:
        async with session.get('https://gatherer.wizards.com/Pages/Card/Details.aspx?multiverseid={0}'.format(cardid)) as response:
            html = await response.text()
    soup = BeautifulSoup(html, 'html.parser')
    flavortTexts = soup.find_all('div', class_='flavortextbox')
    if not flavortTexts:
        return

    text = ""

    for flavortText in flavortTexts:
        if text:
            text = text + '\n' + flavortText.contents[0]
        else:
            text += flavortText.contents[0]
    return text


async def fetchCNText(collectorNumber: str, cardid: int) -> None:
    TSInfo[collectorNumber]['cnFlavor'] = await getCardFlavorText(cardid)


async def fetchENText(collectorNumber: str, cardid: int) -> None:
    TSInfo[collectorNumber]['enFlavor'] = await getCardFlavorText(cardid)


async def main():
    encards = await getCardsInfo('Dominaria United', {
        'CardDatabaseSettings': '0=1&1=28&2=0&14=1&3=13&4=0&5=1&6=15&7=0&8=1&9=1&10=18&11=7&12=8&15=1&16=0&13='})
    cncards = await getCardsInfo('Dominaria United', {
        'CardDatabaseSettings': '0=1&1=zh-CN&2=0&14=1&3=13&4=0&5=1&6=15&7=0&8=1&9=1&10=18&11=7&12=8&15=1&16=0&13='})

    for name, info in encards.items():
        TSInfo[info['collectorNumber']] = {
            'enName': name, 'cnName': None, 'enMultiverseid': info['cardid'],
            'cnMultiverseid': 0, 'enFlavor': None, 'cnFlavor': None
        }

    for name, info in cncards.items():
        TSInfo[info['collectorNumber']]['cnName'] = name
        TSInfo[info['collectorNumber']]['cnMultiverseid'] = info['cardid']

    tasks = []
    for cn, cardInfo in TSInfo.items():
        tasks.append(asyncio.create_task(
            fetchENText(cn, cardInfo['enMultiverseid'])))
        tasks.append(asyncio.create_task(
            fetchCNText(cn, cardInfo['cnMultiverseid'])))

    for task in tasks:
        await task

if __name__ == '__main__':
    loop = asyncio.events.new_event_loop()
    asyncio.events.set_event_loop(loop)
    loop.run_until_complete(main())

    with open('DMUFlavor.json', 'w', encoding='UTF8') as f:
        json.dump(TSInfo, f, ensure_ascii=False)
