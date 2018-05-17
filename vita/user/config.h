#ifndef __CONFIG_H_
#define __CONFIG_H_

typedef struct config_t {
    char lang[16];
} config_t;

extern config_t g_config;

int config_load();
void config_save();

#endif // __CONFIG_H_
