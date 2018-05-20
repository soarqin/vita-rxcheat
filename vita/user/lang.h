#ifndef __LANG_H_
#define __LANG_H_

enum {
    LANG_NAME = 0,
    LANG_DESC0,
    LANG_DESC1,
    LANG_MAINMENU,
    LANG_CHEATS,
    LANG_CHEAT_NOT_FOUND,
    LANG_TROPHIES,
    LANG_TROPHIES_READING,
    LANG_ADVANCE,
    LANG_GRADE_P,
    LANG_GRADE_G,
    LANG_GRADE_S,
    LANG_GRADE_B,
    LANG_HIDDEN,
    LANG_DUMP,
    LANG_DUMPING,
    LANG_CHCLOCKS,
    LANG_CURLANG,
    LANG_CLOCKS,
    LANG_LANGUAGE,
    LANG_SEARCHING,
    LANG_UNLOCKED,
    LANG_UNLOCKED_PLAT,
    LANG_COUNT,
};

void lang_init();
void lang_finish();
int lang_set(int index);
int lang_get_count();
int lang_get_index();
const char *lang_get_name(int idx);
const char *lang_get(int id);

#define LS(id) lang_get(LANG_##id)

#endif // __LANG_H_
