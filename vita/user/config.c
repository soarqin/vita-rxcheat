#include "config.h"

#include <vitasdk.h>

config_t g_config = {};

int config_load() {
    int fd = sceIoOpen("ux0:data/rcsvr/config.bin", SCE_O_RDONLY, 0644);
    if (fd < 0) return -1;
    sceIoRead(fd, &g_config, sizeof(config_t));
    sceIoClose(fd);
    return 0;
}

void config_save() {
    int fd = sceIoOpen("ux0:data/rcsvr/config.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0644);
    if (fd < 0) return;
    sceIoWrite(fd, &g_config, sizeof(config_t));
    sceIoClose(fd);
}
