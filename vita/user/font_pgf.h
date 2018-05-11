#ifndef __FONT_PGF_H_
#define __FONT_PGF_H_

#include <stdint.h>

#define CHAR_CROSS    "\xE2\x95\xB3"
#define CHAR_SQUARE   "\xE2\x96\xA1"
#define CHAR_TRIANGLE "\xE2\x96\xB3"
#define CHAR_CIRCLE   "\xE2\x97\x8B"

#define CHAR_LEFT  "\xE2\x86\x90"
#define CHAR_RIGHT "\xE2\x86\x92"
#define CHAR_UP    "\xE2\x86\x91"
#define CHAR_DOWN  "\xE2\x86\x93"

void font_pgf_init();
void font_pgf_finish();
int font_pgf_char_glyph(uint16_t code, const uint8_t **lines, int *pitch, uint8_t *realw, uint8_t *w, uint8_t *h, uint8_t *l, uint8_t *t);
int font_pgf_loaded();

#endif
