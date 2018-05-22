#ifndef __BLIT_H__
#define __BLIT_H__

#include <psp2/display.h>

#define COLOR_CYAN    0x00ffff00
#define COLOR_MAGENDA 0x00ff00ff
#define COLOR_YELLOW  0x0000ffff

#define RGB(R,G,B)    (((B)<<16)|((G)<<8)|(R))
#define RGBT(R,G,B,T) (((T)<<24)|((B)<<16)|((G)<<8)|(R))

#define CENTER(num) ((960/2)-(num*(16/2)))

extern int blit_pwidth, blit_pheight;

int blit_setup(void);
int blit_set_frame_buf(const SceDisplayFrameBuf *param);
void blit_set_color(uint32_t fg_col);
void blit_clear(int sx, int sy, int w, int h);
void blit_clear_all();
int blit_string(int sx, int sy, int alpha, const char *msg);
int blit_string_ctr(int sy, int alpha, const char *msg);
int blit_stringf(int sx, int sy, int alpha, const char *msg, ...);

#endif
