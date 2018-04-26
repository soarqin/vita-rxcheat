#include "font_pgf.h"

#include "debug.h"

#include <vitasdk.h>
#include <stdlib.h>
#include <sys/tree.h>
#include <taipool.h>

SceFontLibHandle font_lib = NULL;
SceFontHandle font_handle = NULL;
uint8_t font_data[512 * 1024];
uint16_t glyph_index_next = 0;

typedef struct glyph_entry {
    RB_ENTRY(glyph_entry) entry;
    uint16_t code;
    uint16_t index;
    uint8_t realw, w, h, l, t;
} glyph_entry;
glyph_entry entries[51 * 51];
int entry_used = 0;

inline int glyph_entry_compare(glyph_entry *a, glyph_entry *b) {
    return (int)(a->code - b->code);
}

/* Define a tree root type */
RB_HEAD(glyph_map, glyph_entry);

/* Prototype RB tree API functions */
RB_PROTOTYPE(glyph_map, glyph_entry, entry, glyph_entry_compare);

struct glyph_map glyph_charmap = RB_INITIALIZER(&glyph_charmap);

RB_GENERATE(glyph_map, glyph_entry, entry, glyph_entry_compare);

inline uint16_t find_glyph(uint16_t code, uint8_t *realw, uint8_t *w, uint8_t *h, uint8_t *l, uint8_t *t) {
    glyph_entry ge;
    ge.code = code;
    glyph_entry *res = RB_FIND(glyph_map, &glyph_charmap, &ge);
    if (res == NULL) return 0xFFFF;
    *realw = res->realw;
    *w = res->w;
    *h = res->h;
    *l = res->l;
    *t = res->t;
    return res->index;
}

inline uint16_t insert_glyph(uint16_t code, uint8_t realw, uint8_t w, uint8_t h, uint8_t l, uint8_t t) {
    glyph_entry *ge = &entries[entry_used++];
    ge->code = code;
    ge->index = glyph_index_next++;
    ge->realw = realw;
    ge->w = w;
    ge->h = h;
    ge->l = l;
    ge->t = t;
    RB_INSERT(glyph_map, &glyph_charmap, ge);
    return ge->index;
}

inline void free_glyphs() {
    RB_INIT(&glyph_charmap);
    memset(entries, 0, sizeof(entries));
    entry_used = 0;
}

static void *my_alloc(void *data, unsigned int size) {
    return taipool_alloc(size);
}

static void my_free(void *data, void *p) {
    return taipool_free(p);
}

void font_pgf_init() {
    if (sceSysmoduleIsLoaded(SCE_SYSMODULE_PGF) != SCE_SYSMODULE_LOADED)
        sceSysmoduleLoadModule(SCE_SYSMODULE_PGF);
    SceFontNewLibParams params = {
        NULL, 4, NULL, my_alloc, my_free, NULL, NULL, NULL, NULL, NULL, NULL,
    };
    unsigned int err;
    font_lib = sceFontNewLib(&params, &err);
    if (err != SCE_OK) {
        font_lib = NULL;
        log_debug("sceFontNewLib: %08X\n", err);
        return;
    }
    SceFontStyle style = {0};
    style.fontH = 10;
    style.fontV = 10;
    style.fontLanguage = SCE_FONT_LANGUAGE_DEFAULT;
    int index = sceFontFindOptimumFont(font_lib, &style, &err);
    if (err != SCE_OK) {
        sceFontDoneLib(font_lib);
        font_lib = NULL;
        log_debug("sceFontFindOptimumFont: %08X\n", err);
        return;
    }
    log_debug("Found PGF font index: %d\n", index);
    font_handle = sceFontOpen(font_lib, index, 0, &err);
    if (err != SCE_OK) {
        font_handle = NULL;
        sceFontDoneLib(font_lib);
        font_lib = NULL;
        log_debug("sceFontOpen: %08X\n", err);
        return;
    }
}

int font_pgf_char_glyph(uint16_t code, const uint8_t **lines, int *pitch, uint8_t *realw, uint8_t *w, uint8_t *h, uint8_t *l, uint8_t *t) {
    SceFontCharInfo char_info;
    int ret = sceFontGetCharInfo(font_handle, code, &char_info);
    if (ret < 0) {
        sceFontClose(font_handle);
        font_handle = NULL;
        sceFontDoneLib(font_lib);
        font_lib = NULL;
        log_debug("sceFontGetCharInfo: %d\n", ret);
        return -1;
    }
    uint16_t glyph_index = find_glyph(code, realw, w, h, l, t);
    if (glyph_index != 0xFFFF) {
        int sx = (glyph_index % 51) * 10;
        int sy = (glyph_index / 51) * 20;
        *lines = &font_data[512 * sy + sx];
        *pitch = 512;
        return 0;
    }
    *realw = (uint8_t)((char_info.sfp26AdvanceH + 32) / 64);
    *w = (uint8_t)char_info.bitmapWidth;
    *h = (uint8_t)char_info.bitmapHeight;
    *l = (uint8_t)(char_info.bitmapLeft < 0 ? 0 : char_info.bitmapLeft);
    *t = (uint8_t)(char_info.bitmapTop < 0 || char_info.bitmapTop >= 32 ? 0 : char_info.bitmapTop);
    glyph_index = insert_glyph(code, *realw, *w, *h, *l, *t);
    log_debug("insert_glyph: %04X %u %u %u %u %u\n", code, *realw, *w, *h, *l, *t);
    SceFontGlyphImage glyph_image;
    int sx = (glyph_index % 51) * 10;
    int sy = (glyph_index / 51) * 20;
    glyph_image.pixelFormat = SCE_FONT_PIXELFORMAT_4;
    glyph_image.xPos64 = (sx * 2) << 6;
    glyph_image.yPos64 = sy << 6;
    glyph_image.bufWidth = 1024;
    glyph_image.bufHeight = 1024;
    glyph_image.bytesPerLine = 512;
    glyph_image.pad = 0;
    glyph_image.bufferPtr = (unsigned int)font_data;

    ret = sceFontGetCharGlyphImage(font_handle, code, &glyph_image);
    if (ret < 0) {
        log_debug("sceFontGetCharGlyphImage: %d\n", ret);
        return -1;
    }
    *lines = &font_data[512 * sy + sx];
    *pitch = 512;
    return 0;
}

void font_pgf_finish() {
    if (font_lib != NULL) {
        sceFontClose(font_handle);
        font_handle = NULL;
        sceFontDoneLib(font_lib);
        font_lib = NULL;
    }
}

int font_pgf_loaded() {
    return font_lib != NULL;
}
