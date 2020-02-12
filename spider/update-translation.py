import collections
import json
import pathlib
import re

import requests

HEADERS = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.130 Safari/537.36"
}
CACHEDIR = 'jsons'
OUTDIR = 'outputs'
FORGE_RES_PATH = '~/forge/forge-gui/res'


class CardInfo:
    def __init__(self, ename: str, cname: str, maintype: str, subtype: str, rule: str):
        self.ename = ename
        self.cname = cname
        self.maintype = maintype
        self.subtype = subtype
        self.rule = rule

        self.ename = self.ename.strip('\n')
        self.ename = self.ename.strip('\r\n\xa0')
        if self.ename[0].islower():
            self.ename = self.ename[0].swapcase() + self.ename[1:]

        i = self.ename.find('(')
        if i != -1:
            self.ename = self.ename[:i-1]

        if self.cname != "":
            i = self.cname.find('(')
            if i != -1:
                self.cname = self.cname[:i-1]

        if self.rule != "":
            self.rule = self.rule.replace('\r\n\r\n', '\\n')
            self.rule = self.rule.replace('\r\r\n', '\\n')
            self.rule = self.rule.replace('\r\n', '\\n')
            self.rule = self.rule.replace('\n', '\\n')
            self.rule = self.rule.replace('{WP}', '{W/P}')
            self.rule = self.rule.replace('{UP}', '{U/P}')
            self.rule = self.rule.replace('{BP}', '{B/P}')
            self.rule = self.rule.replace('{RP}', '{R/P}')
            self.rule = self.rule.replace('{GP}', '{G/P}')
            self.rule = self.rule.replace('{Red}', '{R}')
            self.rule = self.rule.replace('{Green}', '{G}')
            self.rule = self.rule.replace('{Tap}', '{T}')
            self.rule = self.rule.replace('（', '(')
            self.rule = self.rule.replace('）', ')')

    def parse(self) -> tuple:
        translatepriority = 0
        translate = ""

        if self.cname != "":
            translatepriority += 1
        if self.rule != "":
            translatepriority += 1

        if self.maintype != "":
            translatepriority += 1
        if self.subtype != "":
            translatepriority += 1
            translate = '|{0}|{1}～{2}|{3}'.format(
                self.cname, self.maintype, self.subtype, self.rule)
        else:
            translate = '|{0}|{1}|{2}'.format(
                self.cname, self.maintype, self.rule)
        return (translatepriority, self.ename, translate)


def get_setlist():
    if not pathlib.Path(CACHEDIR).is_dir():
        pathlib.Path(CACHEDIR).unlink()
        pathlib.Path(CACHEDIR).mkdir()
    cachename = "{0}/setlist.json".format(CACHEDIR)
    if pathlib.Path(cachename).is_file():
        with open(cachename, mode='r', encoding='utf8') as setlist:
            sets = json.load(setlist)
            return sets
    resp = requests.get(
        "https://www.iyingdi.com/magic/series/list?size=-1", headers=HEADERS)
    with open(cachename, mode='w', encoding='utf8') as cachefile:
        json.dump(resp.json()['list'], cachefile, ensure_ascii=False)
    return resp.json()['list']


def get_setinfo(setid: int, setcode: str):
    if not pathlib.Path(CACHEDIR).is_dir():
        pathlib.Path(CACHEDIR).unlink()
        pathlib.Path(CACHEDIR).mkdir()
    cachename = "{0}/{1}.json".format(CACHEDIR, setcode)
    if pathlib.Path(cachename).is_file():
        with open(cachename, mode='r', encoding='utf8') as cachefile:
            info = json.load(cachefile)
            return info

    pagesize = 30
    resp = requests.post('https://www.iyingdi.com/magic/card/search/vertical', data={
        'statistic': 'total',
        'token': '7d3dfc94dc9e4378b19390dc757b4bf8',
        'page': 0,
        'size': pagesize,
        'series': setid,
    }, headers=HEADERS)
    setsize = resp.json()['data']['total']

    pagetotal = setsize//pagesize
    if setsize > pagesize*pagetotal:
        pagetotal += 1
    info = []
    for page in range(pagetotal):
        resp = requests.post('https://www.iyingdi.com/magic/card/search/vertical', data={
            'statistic': 'total',
            'token': '7d3dfc94dc9e4378b19390dc757b4bf8',
            'page': page,
            'size': pagesize,
            'series': setid,
        }, headers=HEADERS)
        cards = resp.json()['data']['cards']
        for card in cards:
            info.append(card)
    with open(cachename, mode='w', encoding='utf8') as cachefile:
        json.dump(info, cachefile, ensure_ascii=False)
    return info


def get_oldtranslations(filepath: str):
    #oracle card name|translation card name|translation card type|translation card text
    reg_pattern = re.compile(r"([^|]*)\|([^|]*)\|([^|]*)\|([^|^\n]*)")
    translations = {}
    with open(filepath, mode='r', encoding='utf8') as f:
        for line in f.readlines():
            match = re.match(reg_pattern, line)
            if not match:
                continue
            ename = match.group(1)
            cname = match.group(2)
            cname = cname.replace('“', '"')
            cname = cname.replace('”', '"')
            # ignore invalid translation
            if ename == cname:
                continue
            if ename[0].islower():
                ename = ename[0].swapcase() + ename[1:]
            translations[ename] = "|{0}|{1}|{2}".format(
                match.group(2), match.group(3), match.group(4), )
    return translations


def translation_tofile(translation: collections.OrderedDict, filename: str):
    if not pathlib.Path(CACHEDIR).is_dir():
        pathlib.Path(CACHEDIR).mkdir()
    with open("{0}/{1}".format(OUTDIR, filename),
              mode="w", encoding='utf8') as tsfile:
        for key, value in translation.items():
            tsfile.write('{0}{1}\n'.format(key, value))


if __name__ == "__main__":
    translation_priority = {}
    translation_patch = {}

    oldtranslation = get_oldtranslations(
        "{0}/languages/cardnames-zh-CN.txt".format(FORGE_RES_PATH))
    sets = get_setlist()
    setsize = sets.__len__()
    translatebasepriority = 10**(str(setsize).__len__())
    for mtgset in sets:
        id = mtgset['id']
        abbr = mtgset['abbr']
        setpriority = id

        setinfo = get_setinfo(id, abbr)
        for card in setinfo:
            translatepriority = 0
            enames = []
            cnames = []
            rules = []
            maintype = card['mainType']
            subtype = card['subType']

            if ' // ' in card['ename']:
                # double face card
                enames = card['ename'].split(' // ')
                cnames = card['cname'].split('//')
                rules = card['rule'].split('//')
                maintype = card['mainType']
                subtype = card['subType']
            else:
                enames.append(card['ename'])
                cnames.append(card['cname'])
                rules.append(card['rule'])
                maintype = card['mainType']
                subtype = card['subType']

            index = min(enames.__len__(),
                        cnames.__len__(), rules.__len__())
            for i in range(index):
                translatepriority, key, value = CardInfo(
                    enames[i], cnames[i], maintype, subtype, rules[i]).parse()
                priority = translatebasepriority*translatepriority + setpriority
                if oldtranslation.__contains__(key):
                    continue
                if translation_patch.__contains__(key):
                    if priority <= translation_priority[key]:
                        continue
                translation_priority[key] = priority
                translation_patch[key] = value

    new_translations = {**oldtranslation, **translation_patch}
    translation_patch = collections.OrderedDict(
        sorted(translation_patch.items()))
    new_translations = collections.OrderedDict(
        sorted(new_translations.items()))
    translation_tofile(translation_patch, 'cardnames-zh-CN-patch.txt')
    translation_tofile(new_translations, 'cardnames-zh-CN.txt')
