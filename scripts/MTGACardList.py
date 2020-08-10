import json
import pathlib
import enum

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
        cardname = translations[data['titleId']]
        series = data['set']
        if cardsinfo.get(series, None) != None:
            if cardsinfo[series].get(data['collectorNumber'], None) != None:
                if '///' in cardsinfo[series][data['collectorNumber']]:
                    continue
            cardsinfo[series][data['collectorNumber']] = cardname
        else:
            cardsinfo[series] = {
                data['collectorNumber']: cardname}
    return cardsinfo


def print_cardlist(cardlist: dict):
    """format output card list"""
    for series, seriesinfo in cardlist.items():
        print("{0}".format(series))
        for cardid, cardname in seriesinfo.items():
            print("\t{0}\t{1}".format(cardid, cardname))


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
