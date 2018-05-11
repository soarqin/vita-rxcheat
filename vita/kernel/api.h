#ifndef __KERNEL_API_H__
#define __KERNEL_API_H__

void rcsvrMemcpyForce(void *dstp, const void *srcp, int data_len, int flush);
void rcsvrMemcpy(void *dst, const void *src, int size);

#endif // __KERNEL_API_H__
