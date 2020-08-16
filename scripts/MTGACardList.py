import collections
import enum
import functools
import json
import pathlib
import typing

MTGA_INSTALL_DIR = r'C:/Program Files/Wizards of the Coast/MTGA'


class Languages(enum.Enum):
    EN = 'en-US'
    FR = 'fr-FR'
    IT = 'it-IT'
    DE = 'de-DE'
    ES = 'es-ES'
    JP = 'ja-JP'
    BR = 'pt-BR'
    RU = 'ru-RU'
    KR = 'ko-KR'


class CardRarity(enum.Enum):
    LAND = 1
    COMMON = 2
    UNCOMMON = 3
    RARE = 4
    MYTHICRARE = 5

    def __str__(self):
        if self.value == CardRarity.LAND.value:
            return 'L'
        elif self.value == CardRarity.COMMON.value:
            return 'C'
        elif self.value == CardRarity.UNCOMMON.value:
            return 'U'
        elif self.value == CardRarity.RARE.value:
            return 'R'
        elif self.value == CardRarity.MYTHICRARE.value:
            return 'M'
        else:
            return 'L'


class Card:
    def __init__(self, rarity_value: int, oracle_text: str):
        self.name: str = oracle_text
        self.rarity: CardRarity = CardRarity(rarity_value)

    def __str__(self):
        return "{0} {1}".format(self.rarity, self.name)


def get_cardinfos(data_paths: tuple, lang=Languages.EN) -> dict:
    """get the MTGA card information list"""

    def get_translations(translation_path: pathlib.Path, lang: Languages) -> dict:
        """get the translated text in the specified language by MTGA"""
        if not translation_path.is_file():
            raise FileNotFoundError('can not found mtga translation file')
        datas = None
        with open(translation_path, mode='r', encoding='utf8') as data_file:
            datas = json.load(data_file)
        translations = {}
        for data in datas:
            if data['isoCode'] != lang.value:
                continue
            for translation in data['keys']:
                translations[translation['id']] = translation['text']
        return translations

    def cmp_seriesid(id1: tuple, id2: tuple):
        """
            the `tuple` -> (card_collector_id: str,card_name: str)
        """
        def general_cmp(obj1: typing.Union[int, str], obj2: typing.Union[int, str]):
            """
                basic cmp function
            """
            if obj1 > obj2:
                return 1
            elif obj1 < obj2:
                return -1
            else:
                return 0

        if id1[0].isdigit():
            if id2[0].isdigit():
                value = general_cmp(int(id1[0]), int(id2[0]))
                if value != 0:
                    return value
                else:
                    return general_cmp(id1[1], id2[1])
            else:
                return -1
        else:
            if id2[0].isdigit():
                return 1
            else:
                value = general_cmp(id1[0], id2[0])
                if value != 0:
                    return value
                else:
                    return general_cmp(id1[1], id2[1])

    translations = get_translations(data_paths[0], lang)
    if not data_paths[1].is_file():
        raise FileNotFoundError('can not found mtga card info file')
    datas = None
    with open(data_paths[1], mode='r', encoding='utf8') as data_file:
        datas = json.load(data_file)
    cardsinfo = {}

    for data in datas:
        if data['isToken'] == True:
            continue
        if data['isPrimaryCard'] != True:
            continue
        series = data['set']

        cardrarity = data['rarity']
        cardname = translations[data['titleId']]

        card = Card(cardrarity, cardname)
        if cardsinfo.get(series, None) != None:
            if cardsinfo[series].get(data['collectorNumber'], None) != None:
                if '///' in cardsinfo[series][data['collectorNumber']].name:
                    continue
            cardsinfo[series][data['collectorNumber']] = card
        else:
            cardsinfo[series] = {
                data['collectorNumber']: card}

    sorted_cards = {}
    for series_id, series_cards in cardsinfo.items():
        sorted_cards[series_id] = collections.OrderedDict(
            sorted(series_cards.items(), key=functools.cmp_to_key(cmp_seriesid)))
    return sorted_cards


def print_cardlist(cardlist: dict):
    """format output card list"""
    for series, seriesinfo in cardlist.items():
        print("{0}".format(series))
        for cardid, cardname in seriesinfo.items():
            print("\t{0} {1}".format(cardid, cardname))


def get_newdata_path() -> tuple:
    """get all resource file paths with a specific prefix"""
    resource_dir = pathlib.Path(
        "{0}/MTGA_Data/Downloads/Data/".format(MTGA_INSTALL_DIR))
    new_translation_path = None
    new_cardinfo_path = None
    for resource in resource_dir.iterdir():
        if resource.suffix != '.mtga':
            continue
        if resource.stem.startswith('data_loc'):
            if new_translation_path == None:
                new_translation_path = resource
            elif resource.stat().st_ctime < new_translation_path.stat().st_ctime:
                continue
            else:
                new_translation_path = resource
        elif resource.stem.startswith('data_cards'):
            if new_cardinfo_path == None:
                new_cardinfo_path = resource
            elif resource.stat().st_ctime < new_cardinfo_path.stat().st_ctime:
                continue
            else:
                new_cardinfo_path = resource
    return (new_translation_path, new_cardinfo_path)


if __name__ == "__main__":
    print_cardlist(get_cardinfos(get_newdata_path()))
