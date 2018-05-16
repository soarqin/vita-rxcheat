#ifndef __FONT_PGF_H_
#define __FONT_PGF_H_

#include <stdint.h>

void font_pgf_init();
void font_pgf_finish();
int font_pgf_char_glyph(uint16_t code, const uint8_t **lines, int *pitch, int8_t *realw, int8_t *w, int8_t *h, int8_t *l, int8_t *t);
int font_pgf_loaded();

#endif
