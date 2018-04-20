/*
    PSP VSH 24bpp text bliter
*/

#include "blit.h"

#include "debug.h"
#include "font_pgf.h"

#include <psp2/types.h>
#include <psp2/display.h>
#include <psp2/kernel/clib.h> 

#include <stdarg.h>

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static int pwidth, pheight, bufferwidth, pixelformat;
static uint32_t* vram32;

static uint32_t colors[16];

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//int blit_setup(int sx,int sy,const char *msg,int fg_col,int bg_col)
int blit_setup(void)
{
    SceDisplayFrameBuf param;
    param.size = sizeof(SceDisplayFrameBuf);
    sceDisplayGetFrameBuf(&param, SCE_DISPLAY_SETBUF_IMMEDIATE);

    pwidth = param.width;
    pheight = param.height;
    vram32 = param.base;
    bufferwidth = param.pitch;
    pixelformat = param.pixelformat;

    if( (bufferwidth==0) || (pixelformat!=0)) return -1;

    blit_set_color(0xffffffff, 0xff000000);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// blit text
/////////////////////////////////////////////////////////////////////////////
void blit_set_color(uint32_t fg_col, uint32_t bg_col)
{
    int i;
    for (i = 1; i < 15; ++i) {
#define TCOLOR(c1, c2, a, b) (((c1) * (a) + (c2) * ((b) - (a))) / (b))
        uint32_t a = TCOLOR(fg_col>>24, bg_col>>24, i, 15);
        uint32_t b = TCOLOR((fg_col>>16)&0xFF, (bg_col>>16)&0xFF, i, 15);
        uint32_t g = TCOLOR((fg_col>>8)&0xFF, (bg_col>>8)&0xFF, i, 15);
        uint32_t r = TCOLOR(fg_col&0xFF, bg_col&0xFF, i, 15);
#undef TCOLOR
        colors[i] = (a<<24) | (b<<16) | (g<<8) | r;
    }
    colors[15] = fg_col;
    colors[0] = bg_col;
}

int utf8_to_ucs2(const char *utf8, unsigned int *character)
{
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

/////////////////////////////////////////////////////////////////////////////
// blit text
/////////////////////////////////////////////////////////////////////////////
int blit_string(int sx, int sy, const char *msg)
{
    uint32_t *offset = vram32 + sy * bufferwidth + sx;
    int rw = 0;

    if( (bufferwidth==0) || (pixelformat!=0)) return -1;
    while(*msg != 0) {
        unsigned int wch;
        const uint8_t *lines;
        int pitch;
        uint8_t realw, w, h, l, t;
        msg += utf8_to_ucs2(msg, &wch);
        if (font_pgf_char_glyph(wch, &lines, &pitch, &realw, &w, &h, &l, &t) != 0) continue;
        uint32_t *loffset = offset + (18 - (int)t) * bufferwidth + l;
        int i, j;
        int hw = (w + 1) / 2;
        for (j = 0; j < h; ++j) {
            uint32_t *soffset = loffset;
            const uint8_t *ll = lines;
            for (i = 0; i < hw; ++i) {
                uint8_t c = *ll++;
                soffset[0] = colors[c & 0x0F];
                soffset[1] = colors[c >> 4];
                soffset += 2;
            }
            loffset += bufferwidth;
            lines += pitch;
        }
        offset += realw;
        rw += realw;
    }

    return rw;
}

int blit_stringf(int sx, int sy, const char *msg, ...)
{
    va_list list;
    char string[512];

    va_start(list, msg);
    sceClibVsnprintf(string, 512, msg, list);
    va_end(list);

    return blit_string(sx, sy, string);
}

int blit_set_frame_buf(const SceDisplayFrameBuf *param)
{
    pwidth = param->width;
    pheight = param->height;
    vram32 = param->base;
    bufferwidth = param->pitch;
    pixelformat = param->pixelformat;

    if( (bufferwidth==0) || (pixelformat!=0)) return -1;

    blit_set_color(0xffffffff, 0xff000000);

    return 0;
}
