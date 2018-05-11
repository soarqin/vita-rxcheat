#ifndef __CHEAT_H__
#define __CHEAT_H__

typedef struct cheat_t cheat_t;

int cheat_load(const char *titleid);
void cheat_free();
int cheat_loaded();
void cheat_process();
cheat_t *cheat_get_handle();

#endif // __CHEAT_H__