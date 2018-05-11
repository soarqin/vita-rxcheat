#ifndef __TROPHY_H_
#define __TROPHY_H_

#include <stdint.h>

void trophy_init();
void trophy_finish();
void trophy_hook();
void trophy_unhook();
void trophy_list(void (*cb)(int id, int grade, int hidden, int unlocked, const char *name, const char *desc), void (*cb_end)(int err));
void trophy_unlock(int id, int hidden, void (*cb)(int id, int grade, int hidden, int unlocked, const char *name, const char *desc), void (*cb2)(int ret, int id, int platid));
void trophy_unlock_all(uint32_t *hidden, void (*cb)(int id, int grade, int hidden, int unlocked, const char *name, const char *desc), void (*cb2)(int ret, int id, int platid));

void trophy_test();

#endif
