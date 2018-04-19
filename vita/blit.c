/*
    PSP VSH 24bpp text bliter
*/

#include "blit.h"

#include <psp2/types.h>
#include <psp2/display.h>
#include <psp2/kernel/clib.h> 

#include <stdarg.h>

extern unsigned char msx[];

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static int pwidth, pheight, bufferwidth, pixelformat;
static unsigned int* vram32;

static uint32_t fcolor = 0x00ffffff;
static uint32_t bcolor = 0xff000000;

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

    fcolor = 0x00ffffff;
    bcolor = 0xff000000;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// blit text
/////////////////////////////////////////////////////////////////////////////
void blit_set_color(int fg_col,int bg_col)
{
    fcolor = fg_col;
    bcolor = bg_col;
}

/////////////////////////////////////////////////////////////////////////////
// blit text
/////////////////////////////////////////////////////////////////////////////
int blit_string(int sx,int sy,const char *msg)
{
    int x,y,p;
    int offset;
    char code;
    unsigned char font;
    uint32_t fg_col,bg_col;

    fg_col = fcolor;
    bg_col = bcolor;

//Kprintf("MODE %d WIDTH %d\n",pixelformat,bufferwidth);
    if( (bufferwidth==0) || (pixelformat!=0)) return -1;

    for(x=0;msg[x] && x<(pwidth/16);x++)
    {
        code = msg[x] & 0x7f; // 7bit ANK
        for(y=0;y<8;y++)
        {
            offset = (sy+(y*2))*bufferwidth + sx+x*16;
            font = msx[ code*8 + y ];
            for(p=0;p<8;p++)
            {
                uint32_t color = (font & 0x80) ? fg_col : bg_col;
                vram32[offset] = color;
                vram32[offset + 1] = color;
                vram32[offset + bufferwidth] = color;
                vram32[offset + bufferwidth + 1] = color;
                font <<= 1;
                offset+=2;
            }
        }
    }
    return x;
}

int blit_string_ctr(int sy,const char *msg)
{
    int sx = (960 / 2) - (sceClibStrnlen(msg, 1024) * (16 / 2));
    return blit_string(sx,sy,msg);
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

    fcolor = 0x00ffffff;
    bcolor = 0xff000000;

  return 0;
}
