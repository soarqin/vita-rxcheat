#ifndef __KERNEL_API_H__
#define __KERNEL_API_H__

#include <psp2kern/types.h>
typedef struct SceIoDirent SceIoDirent;

int rcsvrMemcpyForce(void *dstp, const void *srcp, int data_len, int flush);
int rcsvrMemcpy(void *dst, const void *src, int size);
SceUID rcsvrIoDopen(const char *dirname);
int rcsvrIoDread(SceUID fd, SceIoDirent *dir);
int rcsvrIoDclose(SceUID fd);

#endif // __KERNEL_API_H__
