#ifndef __CHEAT_H__
#define __CHEAT_H__

int cheat_load(const char *titleid);
void cheat_free();
int cheat_loaded();
void cheat_process();

#endif // __CHEAT_H__