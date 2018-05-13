#ifndef __TROPHY_H_
#define __TROPHY_H_

#include <stdint.h>

typedef struct trophy_info {
    uint8_t id;
    uint8_t grade;
    uint8_t hidden;
    uint8_t unlocked;
    char name[60];
    char desc[128];
} trophy_info;

typedef void (*trophy_info_cb_t)(int id, int grade, int hidden, int unlocked, const char *name, const char *desc);
typedef void (*trophy_info_cb_end_t)(int err);
typedef void (*trophy_unlock_cb_t)(int ret, int id, int platid);

void trophy_init();
void trophy_finish();
int trophy_get_status();
int trophy_get_count();
int trophy_get_info(const trophy_info **trophies);
void trophy_list(trophy_info_cb_t cb, trophy_info_cb_end_t cb_end);
void trophy_unlock(int id, int hidden, trophy_info_cb_t cb, trophy_unlock_cb_t cb2);
void trophy_unlock_all(uint32_t *hidden, trophy_info_cb_t cb, trophy_unlock_cb_t cb2);

void trophy_test();

#endif
