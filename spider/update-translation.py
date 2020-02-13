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


class TSInfo:
    def __init__(self, tsname: str, tstype: str, tstext: str):
        self.name = tsname
        self.type = tstype
        self.text = tstext

    def __str__(self):
        return "|{0}|{1}|{2}".format(self.name, self.type, self.text)


def get_iyingditranslations():
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

    def parse_cardinfo(ename: str, cname: str, maintype: str, subtype: str, rule: str) -> tuple:
        ename = ename.strip('\n')
        ename = ename.strip('\r\n\xa0')
        if ename[0].islower():
            ename = ename[0].swapcase() + ename[1:]

        i = ename.find('(')
        if i != -1:
            ename = ename[:i-1]

        if cname != "":
            i = cname.find('(')
            if i != -1:
                cname = cname[:i-1]

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
        return None

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
                return None
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
                    if 'Name:' in line:
                        line = line.strip('\n')
                        oracleinfo['name'].append(line.replace('Name:', ''))
                    elif 'Types:' in line:
                        line = line.strip('\n')
                        oracleinfo['type'].append(line.replace('Types:', ''))
                    elif 'Oracle:' in line:
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
                if oracle_text[-1:] != '\n':
                    oracle_text += '\n'
                oracle[name] = ('|{0}|{1}|{2}'.format(
                    name, type, oracle_text))
            return oracle

        translations = {}
        for cardoracle in cardoracle_iter():
            translations.update(parse_cardoracle(cardoracle))
        translations = collections.OrderedDict(
            sorted(translations.items()))
        with out_path.open('w', encoding='utf-8', newline='\n') as oracle_file:
            for key, value in translations.items():
                oracle_file.write('{0}{1}'.format(key, value))
        return translations

    if not pathlib.Path(OUTDIR).is_dir():
        pathlib.Path(OUTDIR).unlink()
        pathlib.Path(OUTDIR).mkdir()

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
    tsrule = {
        'Plainswalk': '平原行者',
        'plainswalk': '平原行者',
        'Islandwalk': '海岛行者',
        'islandwalk': '海岛行者',
        'Mountainwalk': '山脉行者',
        'mountainwalk': '山脉行者',
        'Forestwalk': '树林行者',
        'forestwalk': '树林行者',
        'Swampwalk': '沼泽行者',
        'swampwalk': '沼泽行者',
        'First strike': '先攻',
        'Flying': '飞行',
        'shroud': '帷幕',
        'This permanent can\'t be the target of spells or abilities.': '此永久物不能成为咒语或异能的目标',
        'It can\'t be the target of spells or abilities.': '其不能成为咒语或异能的目标',
        'Activate this ability only any time you could cast a sorcery': '只能在你可以释放法术的时机起动此异能',
        'vigilance': '警戒',
        'Vigilance': '警戒',
        'Enchant creature': '结附于生物',
        'Enchant land': '结附于地',
        'Threshold': '门槛',
        'If seven or more cards are in your graveyard': '如果你坟墓场中有七张或者更多的牌',
        'Activate this ability only if seven or more cards are in your graveyard': '只能在你的坟墓场中有七张或更多的牌时起动',
        ' as long as seven or more cards are in your graveyard.': '只要在你的坟墓场中有七张或更多的牌。',
        'Fading': '消退',
        ', remove a fade counter from it. If you can\'t, sacrifice it.': '从其上移去一个消退指示物。若你无法如此作，则牺牲该永久物',
        'At the beginning of your upkeep': '在你的维持开始时',
        'At the beginning of each player\'s upkeep': '在每位牌手的维持开始时',
        ' enters the battlefield,': '进入战场，',
        'Draw a card.': '抓一张。',
        'draw a card.': '抓一张。',
        'Protection from artifacts': '反神器保护',
        'protection from artifacts': '反神器保护',
        'protection from blue': '反蓝保护',
        'Protection from blue': '反蓝保护',
        'protection from red': '反红保护',
        'Protection from red': '反红保护',
        'protection from white': '反白保护',
        'Protection from white': '反白保护',
        'protection from black': '反黑保护',
        'Protection from black': '反黑保护',
        'protection from green': '反绿保护',
        'Protection from green': '反绿保护',
        'Protection from creatures': '反生物保护',
        'Trample': '践踏',
        'haste': '敏捷',
        'Haste': '敏捷',
        'You may cast this face down as a 2/2 creature for {3}. Turn it face up any time for its morph cost.': '你可牌面朝下地使用此牌并支付{3}，将其当成2/2生物。 可随时支付其变身费用使其翻回正面。',
        'Morph': '变身',
        'Kicker': '增幅',
        'Flashback': '返照',
        'You may cast this card from your graveyard for its flashback cost. Then exile it.': '你可以从你的坟墓场施放此牌，并支付其返照费用，然后将它放逐。',
        'Flash': '闪现',
        'counter target spell.': '反击目标咒语。',
        'Cycling ': '循环',
        'Discard this card: ': '弃掉此牌：',
        'This creature can\'t be blocked except by creatures with horsemanship.': '此生物不能被不具有马术的生物阻挡',
        'Horsemanship': '马术',
        'horsemanship': '马术',
        'Defender': '守军',
        'This creature can\'t attack.': '此生物不能攻击',
        'Choose one —': '选择一项',
        'Regenerate ': '重生',
        'Discard a card:': '弃一张牌：',
        'Echo ': '返响',
        ', if this came under your control since the beginning of your last upkeep, sacrifice it unless you pay its echo cost.': '，如果其是从你最近一个维持开始操控的，则除非你支付其返响费用，否则牺牲之。',
        'It can\'t be regenerated.': '其不能重生。',
        'Sacrifice a land': '牺牲一个地',
        'Rampage': '狂暴',
        'rampage': '狂暴',
        'Whenever this creature becomes blocked,': '当此生物被阻挡，',
        ' gets +1/+1 until end of turn.': '获得+1/+1直到回合结束。',
        'Tap target creature.': '横置目标生物。',
        'All creatures have ': '所有生物具有',
        'All creatures get ': '所有生物获得',
        'Cumulative Upkeep': '累积维持',
        'Cumulative upkeep': '累积维持',
        'Deathtouch': '死触',
        'Double strike ': '连击',
        'This creature deals both first-strike and regular combat damage.': '该生物造成先攻和常规战斗伤害。',
        'Hexproof ': '辟邪',
        'This creature can\'t be the target of spells or abilities your opponents control.': '此生物不能成为你对手所操控的咒语或异能的目标',
        'Lifelink': '系命',
        'reach': '延势',
        'Reach': '延势',
        'This creature can block creatures with flying.': '此生物可以阻挡飞行生物',
        'Any creatures with banding, and up to one without, can attack in a band. Bands are blocked as a group. If any creatures with banding ' +
        'a player controls are blocking or being blocked by a creature, that player divides that creature\'s combat damage, not its controller' +
        ', among any of the creatures it\'s being blocked by or is blocking.': '一个或数个具有结合异能的攻击生物，以及至多一个不具结合异能的攻击生物可以结合成为一个团队进行攻击。' +
        '如果团队中任意一个攻击生物被一个生物所阻挡，则该阻挡生物也同时阻挡了该攻击生物所在团队中的所有其他生物。如果由你控制的任意具有结合' +
        '异能的生物正在阻挡某生物或者被某生物阻挡则由你对该生物造成的伤害进行分配而不是其操控者。',
        'Any creatures with banding, and up to one without, can attack in a band. Bands are blocked as a group. If any creatures with banding ' +
        'you control are blocking or being blocked by a creature, you divide that creature\'s combat damage, not its controller, among any of ' +
        'the creatures it\'s being blocked by or is blocking.': '一个或数个具有结合异能的攻击生物，以及至多一个不具结合异能的攻击生物可以结合成为一个团队进行攻击。' +
        '如果团队中任意一个攻击生物被一个生物所阻挡，则该阻挡生物也同时阻挡了该攻击生物所在团队中的所有其他生物。如果由你控制的任意具有结合' +
        '异能的生物正在阻挡某生物或者被某生物阻挡则由你对该生物造成的伤害进行分配而不是其操控者。',
        'Banding ': '结合',
        'banding': '结合',
        'All Sliver creatures have': '所有裂片妖具有',
        'Amplify': '增强',
        'Enchanted creature has ': '所结附的生物具有',
        'Sacrifice a creature:': '牺牲一个生物',
        'Until end of turn': '直到回合结束',
        'until end of turn': '直到回合结束',
        'Destroy target land': '消灭目标地',
        'destroy target land': '消灭目标地',
        'Sacrifice ': '牺牲',
        'sacrifice ': '牺牲',
        'Exile ': '放逐',
        'Storm ': '风暴',
        'When you cast this spell, copy it for each spell cast before it this turn. You may choose new targets for the copies.': '当你施放此咒语时，本回合于此咒语之前每施放过一个咒语，便复制该咒语一次。若此咒语需要目标，你可以为任意复制品选择新的目标。',
        'Target creature can\'t be blocked this turn.': '目标生物本回合不能进行阻挡。',
        ' can\'t be blocked.': '不能被阻挡。',
        'you may pay ': '你可以支付',
        'Target creature gains first strike ': '目标生物获得先攻',
        'Draw a card at the beginning of the next turn\'s upkeep.': '在下个回合的维持开始时抓一张。',
        'Pay 1 life': '支付一点生命',
        'When ': '当'
    }
    for key, value in translation.items():
        # oracle card name to translation card name
        value.text = value.text.replace(key, value.name)
        for ts_key, ts_value in tsrule.items():
            value.text = value.text.replace(ts_key, ts_value)


def translation_tofile(translation: collections.OrderedDict, filename: str):
    if not pathlib.Path(OUTDIR).is_dir():
        pathlib.Path(OUTDIR).unlink()
        pathlib.Path(OUTDIR).mkdir()
    with open("{0}/{1}".format(OUTDIR, filename),
              mode="w", encoding='utf8') as tsfile:
        for key, value in translation.items():
            tsfile.write('{0}{1}\n'.format(key, str(value)))


if __name__ == "__main__":
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
