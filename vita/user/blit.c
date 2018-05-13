/*
    PSP VSH 24bpp text bliter
*/

#include "blit.h"

#include "font_pgf.h"

#include <psp2/types.h>
#include <psp2/display.h>
#include <psp2/kernel/clib.h> 

#include <stdarg.h>

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int pwidth, pheight, bufferwidth, pixelformat;
uint32_t* vram32;

uint32_t alphas[16] = {
    0, 0x10, 0x20, 0x30,
    0x40, 0x50, 0x60, 0x70,
    0x80, 0x90, 0xA0, 0xB0,
    0xC0, 0xD0, 0xE0, 0x100,
};
uint32_t fgcolor;
typedef struct wchar_info {
    const uint8_t *lines;
    int pitch;
    uint16_t wch;
    int8_t realw, w, h, l, t;
} wchar_info;
static wchar_info wci[128];

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//int blit_setup(int sx,int sy,const char *msg,int fg_col,int bg_col)
int blit_setup(void) {
    SceDisplayFrameBuf param;
    param.size = sizeof(SceDisplayFrameBuf);
    sceDisplayGetFrameBuf(&param, SCE_DISPLAY_SETBUF_IMMEDIATE);

    pwidth = param.width;
    pheight = param.height;
    vram32 = param.base;
    bufferwidth = param.pitch;
    pixelformat = param.pixelformat;

    if (bufferwidth == 0 || pixelformat != 0) return -1;
    blit_set_color(0xffffffff);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// blit text
/////////////////////////////////////////////////////////////////////////////
void blit_set_color(uint32_t fg_col) {
    fgcolor = fg_col;
}

inline int utf8_to_ucs2(const char *utf8, uint16_t *character) {
    if (((utf8[0] & 0xF0) == 0xE0) && ((utf8[1] & 0xC0) == 0x80) && ((utf8[2] & 0xC0) == 0x80)) {
        *character = ((utf8[0] & 0x0F) << 12) | ((utf8[1] & 0x3F) << 6) | (utf8[2] & 0x3F);
        return 3;
    } else if (((utf8[0] & 0xE0) == 0xC0) && ((utf8[1] & 0xC0) == 0x80)) {
        *character = ((utf8[0] & 0x1F) << 6) | (utf8[1] & 0x3F);
        return 2;
    } else {
        *character = utf8[0];
        return 1;
    }
}

inline uint32_t alpha_blend(uint32_t c1, uint32_t c2, uint32_t alpha) {
    if (alpha == 0) return c2;
    if (alpha == 256) return c1;
    uint32_t crb = (((c1 & 0xFF00FF) * alpha + (c2 & 0xFF00FF) * (256 - alpha)) >> 8) & 0xFF00FF;
    uint32_t cg = (((c1 & 0xFF00) * alpha + (c2 & 0xFF00) * (256 - alpha)) >> 8) & 0xFF00;
    return 0xFF000000U | cg | crb;
}

inline void blit_stringw(int sx, int sy, const wchar_info *wci) {
    uint32_t *offset = vram32 + sy * bufferwidth + sx;
    const wchar_info *pwci = wci;
    while(pwci->wch != 0) {
        const uint8_t *lines = pwci->lines;
        uint32_t *loffset = offset + (18 - (int)pwci->t) * bufferwidth + pwci->l;
        int i, j;
        int hw = (pwci->w + 1) / 2;
        for (j = 0; j < pwci->h; ++j) {
            uint32_t *soffset = loffset;
            const uint8_t *ll = lines;
            for (i = 0; i < hw; ++i) {
                uint8_t c = *ll++;
                soffset[0] = alpha_blend(fgcolor, soffset[0], alphas[c & 0x0F]);
                soffset[1] = alpha_blend(fgcolor, soffset[1], alphas[c >> 4]);
                soffset += 2;
            }
            loffset += bufferwidth;
            lines += pwci->pitch;
        }
        offset += pwci->realw;
        ++pwci;
    }
}

/////////////////////////////////////////////////////////////////////////////
// blit text
/////////////////////////////////////////////////////////////////////////////
int blit_string(int sx, int sy, const char *msg) {
    if (bufferwidth == 0 || pixelformat != 0) return -1;

    int rw = 0;
    wchar_info *pwci = wci;
    while(*msg != 0) {
        msg += utf8_to_ucs2(msg, &pwci->wch);
        if (font_pgf_char_glyph(pwci->wch, &pwci->lines, &pwci->pitch, &pwci->realw, &pwci->w, &pwci->h, &pwci->l, &pwci->t) != 0) continue;
        rw += pwci->realw;
        ++pwci;
    }
    pwci->wch = 0;
    blit_stringw(sx, sy, wci);
    return rw;
}

int blit_string_ctr(int sy, const char *msg) {
    if (bufferwidth == 0 || pixelformat != 0) return -1;
    int rw = 0;
    wchar_info *pwci = wci;
    while(*msg != 0) {
        msg += utf8_to_ucs2(msg, &pwci->wch);
        if (font_pgf_char_glyph(pwci->wch, &pwci->lines, &pwci->pitch, &pwci->realw, &pwci->w, &pwci->h, &pwci->l, &pwci->t) != 0) continue;
        rw += pwci->realw;
        ++pwci;
    }
    int sx = pwidth / 2 - rw / 2;
    pwci->wch = 0;
    blit_stringw(sx, sy, wci);

    return rw;
}

int blit_stringf(int sx, int sy, const char *msg, ...) {
    va_list list;
    char string[256];

    va_start(list, msg);
    sceClibVsnprintf(string, 256, msg, list);
    va_end(list);

    return blit_string(sx, sy, string);
}

int blit_set_frame_buf(const SceDisplayFrameBuf *param) {
    pwidth = param->width;
    pheight = param->height;
    vram32 = param->base;
    bufferwidth = param->pitch;
    pixelformat = param->pixelformat;

    if (bufferwidth == 0 || pixelformat != 0) return -1;

    return 0;
}
