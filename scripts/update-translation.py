import collections
import json
import pathlib
import re
import time

import requests

HEADERS = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.130 Safari/537.36"
}
CACHEDIR = 'jsons'
OUTDIR = 'outputs'
FORGE_RES_PATH = '~/forge/forge-gui/res'


class TSInfo:
    def __init__(self, tsname: str, tstype: str, tstext: str):
        self.name = tsname
        self.type = tstype
        self.text = tstext

    def __str__(self):
        return "|{0}|{1}|{2}".format(self.name, self.type, self.text)


def get_iyingditranslations():
    def get_setlist():
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

    def parse_cardinfo(ename: str, cname: str, maintype: str, subtype: str, rule: str) -> tuple:
        ename = ename.strip('\n')
        ename = ename.strip('\r\n\xa0')
        if ename[0].islower():
            ename = ename[0].swapcase() + ename[1:]

        i = ename.find('(')
        if i != -1:
            if ename[i-1] == ' ':
                # "Supply (Supply/Demand)" to "Supply"
                ename = ename[:i-1]
            else:
                # "Forest(Theros)" to "Forest"
                ename = ename[:i]

        if cname != "":
            i = cname.find('(')
            if i != -1:
                if cname[i-1] == ' ':
                    cname = cname[:i-1]
                else:
                    cname = cname[:i]

        if rule != "":
            rule = rule.replace('\r\n\r\n', '\\n')
            rule = rule.replace('\r\r\n', '\\n')
            rule = rule.replace('\r\n', '\\n')
            rule = rule.replace('\n', '\\n')
            rule = rule.replace('{WP}', '{W/P}')
            rule = rule.replace('{UP}', '{U/P}')
            rule = rule.replace('{BP}', '{B/P}')
            rule = rule.replace('{RP}', '{R/P}')
            rule = rule.replace('{GP}', '{G/P}')
            rule = rule.replace('{Red}', '{R}')
            rule = rule.replace('{Green}', '{G}')
            rule = rule.replace('{Tap}', '{T}')
            rule = rule.replace('（', '(')
            rule = rule.replace('）', ')')
            rule = rule.replace('「', '"')
            rule = rule.replace('」', '"')
            rule = rule.replace(ename, cname)

        translatepriority = 0

        if cname != "":
            translatepriority += 1
        else:
            cname = ename
        if rule != "":
            translatepriority += 1
        cardtype = ""
        if maintype != "":
            translatepriority += 1
            cardtype += maintype
        if subtype != "":
            translatepriority += 1
            cardtype += "～{0}".format(subtype)
        return (translatepriority, ename, TSInfo(cname, cardtype, rule))

    translation_priority = {}
    translations = {}
    sets = get_setlist()
    setsize = sets.__len__()
    translatebasepriority = 10**(str(setsize).__len__())
    for mtgset in sets:
        id = mtgset['id']
        abbr = mtgset['abbr']
        setpriority = id

        setinfo = get_setinfo(id, abbr)
        # avoid banned
        time.sleep(1)
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
                translatepriority, key, value = parse_cardinfo(
                    enames[i], cnames[i], maintype, subtype, rules[i])
                priority = translatebasepriority*translatepriority + setpriority
                if translations.__contains__(key):
                    if priority <= translation_priority[key]:
                        continue
                translation_priority[key] = priority
                translations[key] = value

    return translations


def get_forgetranslations():
    translations_path = pathlib.Path(
        "{0}/languages/cardnames-zh-CN.txt".format(FORGE_RES_PATH))
    if not translations_path.is_file():
        raise FileNotFoundError("Forge card translations not found")

    reg_pattern = re.compile(r"([^|]*)\|([^|]*)\|([^|]*)\|([^|^\n]*)")
    translations = {}
    with translations_path.open(mode='r', encoding='utf8') as f:
        for line in f.readlines():
            match = re.match(reg_pattern, line)
            if not match:
                continue
            ename = match.group(1)
            cname = match.group(2)
            cname = cname.replace('“', '"')
            cname = cname.replace('”', '"')
            # ignore invalid translation
            if not cname:
                continue
            if ename == cname:
                continue
            if ename[0].islower():
                ename = ename[0].swapcase() + ename[1:]
            translations[ename] = TSInfo(
                match.group(2), match.group(3), match.group(4))
    return translations


def get_oracle():
    def generate_oracle(out_path: pathlib.Path):
        def cardoracle_iter():
            # res/
            #   ...
            #   cardsfolder/
            #       ...
            #       c/
            #           ...
            #           commit_memory.txt
            #           ...
            #       ...
            #       o/
            #           ...
            #           opt.txt
            #           ...
            #       ...
            #   ...
            oracle_dir = pathlib.Path(
                "{0}/cardsfolder/".format(FORGE_RES_PATH))
            if not oracle_dir.is_dir():
                raise FileNotFoundError("Forge cardsfolder not found")
            for carddir in oracle_dir.iterdir():
                if carddir.is_dir():
                    for card in carddir.iterdir():
                        if '.txt' == card.suffix:
                            yield card

        def parse_cardoracle(cardfile: pathlib.Path):
            oracle = {}
            oracleinfo = {
                'name': [],
                'type': [],
                'oracle': []
            }
            with cardfile.open("r", encoding='utf8') as f:
                for line in f.readlines():
                    if line[0:5] == 'Name:':
                        line = line.strip('\n')
                        oracleinfo['name'].append(line.replace('Name:', ''))
                    elif line[0:6] == 'Types:':
                        line = line.strip('\n')
                        oracleinfo['type'].append(line.replace('Types:', ''))
                    elif line[0:7] == 'Oracle:':
                        oracleinfo['oracle'].append(
                            line.replace('Oracle:', ''))
            for index in range(oracleinfo['name'].__len__()):
                name = oracleinfo['name'][index]
                try:
                    type = oracleinfo['type'][index]
                except IndexError:
                    type = ''
                try:
                    oracle_text = oracleinfo['oracle'][index]
                except IndexError:
                    oracle_text = ''
                if oracle_text[-1:] == '\n':
                    oracle_text = oracle_text[1:-1]
                oracle[name] = TSInfo(name, type, oracle_text)
            return oracle

        translations = {}
        for cardoracle in cardoracle_iter():
            translations.update(parse_cardoracle(cardoracle))
        translations = collections.OrderedDict(
            sorted(translations.items()))
        with out_path.open('w', encoding='utf-8', newline='\n') as oracle_file:
            for key, value in translations.items():
                oracle_file.write('{0}{1}\n'.format(key, value))
        return translations

    reg_pattern = re.compile(r"([^|]*)\|([^|]*)\|([^|]*)\|([^|^\n]*)")
    translations = {}

    oracle_path = pathlib.Path("{0}/oracle.txt".format(OUTDIR))
    if not oracle_path.is_file():
        return generate_oracle(oracle_path)

    with oracle_path.open(mode='r', encoding='utf8') as f:
        for line in f.readlines():
            match = re.match(reg_pattern, line)
            if not match:
                continue
            ename = match.group(1)
            translations[ename] = TSInfo(ename, match.group(3), match.group(4))
    return translations


def pre_translation(translation: collections.OrderedDict):
    rule_path = pathlib.Path("{0}/translation_rules.json".format(CACHEDIR))
    if not rule_path.exists():
        return None
    tsrule = {}
    with rule_path.open('r', encoding='utf8') as rulefile:
        tsrule = json.load(rulefile)
    for key, value in translation.items():
        # oracle card name to translation card name
        value.text = value.text.replace(key, value.name)
        for ts_key, ts_value in tsrule.items():
            value.text = value.text.replace(ts_key, ts_value)


def translation_tofile(translation: collections.OrderedDict, filename: str):
    with open("{0}/{1}".format(OUTDIR, filename),
              mode="w", encoding='utf8') as tsfile:
        for key, value in translation.items():
            tsfile.write('{0}{1}\n'.format(key, str(value)))


if __name__ == "__main__":
    out_dir = pathlib.Path(OUTDIR)
    if not out_dir.is_dir():
        if out_dir.exists():
            pathlib.Path(OUTDIR).unlink()
        pathlib.Path(OUTDIR).mkdir()

    cache_dir = pathlib.Path(OUTDIR)
    if not cache_dir.is_dir():
        if cache_dir.exists():
            cache_dir.unlink()
        cache_dir.mkdir()

    if not pathlib.Path(FORGE_RES_PATH).is_dir():
        raise FileNotFoundError("Forge resources directory not found")

    iyingdi_translation = get_iyingditranslations()
    forge_translation = get_forgetranslations()
    oracletranslation = get_oracle()

    translation_patch = {}
    for key, value in iyingdi_translation.items():
        if not forge_translation.__contains__(key):
            translation_patch[key] = value

    new_translations = {**forge_translation, **translation_patch}
    for key, value in oracletranslation.items():
        if not new_translations.__contains__(key):
            translation_patch[key] = value
    pre_translation(translation_patch)
    new_translations = {**forge_translation, **translation_patch}

    translation_patch = collections.OrderedDict(
        sorted(translation_patch.items()))
    new_translations = collections.OrderedDict(
        sorted(new_translations.items()))
    translation_tofile(translation_patch, 'cardnames-zh-CN-patch.txt')
    translation_tofile(new_translations, 'cardnames-zh-CN.txt')
