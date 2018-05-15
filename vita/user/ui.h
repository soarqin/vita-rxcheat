#ifndef __UI_H__
#define __UI_H__

#include <stdint.h>

void ui_init();
void ui_finish();
void ui_process(uint64_t tick);
void ui_set_show_msg(uint64_t millisec, int count, ...);
void ui_clear_show_msg();
void ui_check_msg_timeout(uint64_t curr_tick);

#endif // __UI_H__
