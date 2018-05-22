#include "font_pgf.h"

#include "debug.h"
#include "util.h"

#include <vitasdk.h>
#include <stdlib.h>
#include <sys/tree.h>

static SceFontLibHandle font_lib = NULL;
static SceFontHandle font_handle = NULL;
static uint8_t *font_data = NULL;
static uint16_t glyph_index_next = 0;

typedef struct glyph_entry {
    RB_ENTRY(glyph_entry) entry;
    uint16_t code;
    uint16_t index;
    int8_t realw, w, h, l, t;
} glyph_entry;
static glyph_entry *entries = NULL;
static int entry_used = 0;

static inline int glyph_entry_compare(glyph_entry *a, glyph_entry *b) {
    return (int)(a->code - b->code);
}

/* Define a tree root type */
RB_HEAD(glyph_map, glyph_entry);

/* Prototype RB tree API functions */
RB_PROTOTYPE(glyph_map, glyph_entry, entry, glyph_entry_compare);

struct glyph_map glyph_charmap = RB_INITIALIZER(&glyph_charmap);

RB_GENERATE(glyph_map, glyph_entry, entry, glyph_entry_compare);

static inline uint16_t find_glyph(uint16_t code, int8_t *realw, int8_t *w, int8_t *h, int8_t *l, int8_t *t) {
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

static inline uint16_t insert_glyph(uint16_t code, int8_t realw, int8_t w, int8_t h, int8_t l, int8_t t) {
    if (entry_used >= 51 * 51 * 2) return 0;
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

static inline void free_glyphs() {
    RB_INIT(&glyph_charmap);
    sceClibMemset(entries, 0, sizeof(glyph_entry) * 51 * 51 * 2);
    entry_used = 0;
}

static void *my_pgf_alloc(void *data, unsigned int size) {
    return my_alloc(size);
}

static void my_pgf_free(void *data, void *p) {
    my_free(p);
}

enum {
    SCE_FONT_ERROR_FILEOPEN  = 0x80460005,
    SCE_FONT_ERROR_FILECLOSE = 0x80460006,
    SCE_FONT_ERROR_FILEREAD  = 0x80460007,
    SCE_FONT_ERROR_FILESEEK  = 0x80460008,
};

static void *my_open(void *data, const char *filename, int *errCode) {
    int fd = sceIoOpen(filename, SCE_O_RDONLY, 0644);
    if (fd < 0) {
        if (errCode) *errCode = SCE_FONT_ERROR_FILEOPEN;
        return NULL;
    }
    if (errCode) *errCode = SCE_OK;
    return (void*)(uintptr_t)fd;
}
static int my_close(void *data, void *file) {
    int ret = sceIoClose((uintptr_t)file);
    if (ret < 0) return SCE_FONT_ERROR_FILECLOSE;
    return SCE_OK;
}

static uint32_t my_read(void *data, void *file, void *ptr, uint32_t size, uint32_t num, int *errCode) {
    int r = sceIoRead((uintptr_t)file, ptr, size * num);
    if (r < 0) {
        if (errCode) *errCode = SCE_FONT_ERROR_FILEREAD;
        return 0;
    }
    if (errCode) *errCode = SCE_OK;
    return r / size;
}

static int my_seek(void *data, void *file, int offset) {
    int pos = sceIoLseek((uintptr_t)file, offset, SCE_SEEK_SET);
    if (pos < 0 || pos != offset) return SCE_FONT_ERROR_FILESEEK;
    return SCE_OK;
}

static int open_font_handle() {
    unsigned int err;
    if (font_handle != NULL) {
        sceFontClose(font_handle);
        font_handle = NULL;
    }
    font_handle = sceFontOpenUserFile(font_lib, "ux0:data/rcsvr/font.pgf", 0, &err);
    if (err == SCE_OK) return 0;
    log_trace("sceFontOpenUserFile: %08X, try use internal font\n", err);
    SceFontStyle style = {0};
    style.fontH = 10;
    style.fontV = 10;
    style.fontLanguage = SCE_FONT_LANGUAGE_DEFAULT;
    int index = sceFontFindOptimumFont(font_lib, &style, &err);
    if (err != SCE_OK) {
        sceFontDoneLib(font_lib);
        font_lib = NULL;
        log_error("sceFontFindOptimumFont: %08X\n", err);
        return -1;
    }
    log_trace("Found PGF font index: %d\n", index);
    font_handle = sceFontOpen(font_lib, index, 0, &err);
    if (err != SCE_OK) {
        font_handle = NULL;
        sceFontDoneLib(font_lib);
        font_lib = NULL;
        log_error("sceFontOpen: %08X\n", err);
        return -1;
    }
    return 0;
}

void font_pgf_init() {
    if (font_lib != NULL) return;
    if (sceSysmoduleIsLoaded(SCE_SYSMODULE_PGF) != SCE_SYSMODULE_LOADED)
        sceSysmoduleLoadModule(SCE_SYSMODULE_PGF);
    SceFontNewLibParams params = {
        NULL, 4, NULL, my_pgf_alloc, my_pgf_free, my_open, my_close, my_read, my_seek, NULL, NULL,
    };
    unsigned int err;
    font_lib = sceFontNewLib(&params, &err);
    if (err != SCE_OK) {
        font_lib = NULL;
        log_error("sceFontNewLib: %08X\n", err);
        return;
    }
    if (open_font_handle() < 0) return;
    font_data = (uint8_t*)my_alloc(512 * 2048);
    entries = (glyph_entry*)my_alloc(sizeof(glyph_entry) * 51 * 51 * 2);
}

int font_pgf_char_glyph(uint16_t code, const uint8_t **lines, int *pitch, int8_t *realw, int8_t *w, int8_t *h, int8_t *l, int8_t *t) {
    if (font_handle == NULL) return -1;
    SceFontCharInfo char_info;
    uint16_t glyph_index = find_glyph(code, realw, w, h, l, t);
    if (glyph_index != 0xFFFF) {
        int sx = (glyph_index % 51) * 10;
        int sy = (glyph_index / 51) * 20;
        *lines = &font_data[512 * sy + sx];
        *pitch = 512;
        return 0;
    }
    int ret = sceFontGetCharInfo(font_handle, code, &char_info);
    if (ret < 0) {
        // Reopen font after power resume
        if (open_font_handle() < 0 || (ret = sceFontGetCharInfo(font_handle, code, &char_info)) < 0) {
            sceFontClose(font_handle);
            font_handle = NULL;
            sceFontDoneLib(font_lib);
            font_lib = NULL;
            log_error("sceFontGetCharInfo: %d\n", ret);
        }
        return -1;
    }
    *realw = (int8_t)((char_info.sfp26AdvanceH + 32) / 64);
    *w = (int8_t)char_info.bitmapWidth;
    *h = (int8_t)char_info.bitmapHeight;
    *l = (int8_t)(char_info.bitmapLeft < 0 ? 0 : char_info.bitmapLeft);
    *t = (int8_t)(char_info.bitmapTop < 0 || char_info.bitmapTop >= 32 ? 0 : char_info.bitmapTop);
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
        log_error("sceFontGetCharGlyphImage: %d\n", ret);
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
    if (font_data != NULL) {
        my_free(font_data);
        font_data = NULL;
    }
    if (entries != NULL) {
        my_free(entries);
        entries = NULL;
    }
}

int font_pgf_loaded() {
    return font_lib != NULL;
}
