#ifndef __KERNEL_STRING_H
#define __KERNEL_STRING_H

#define NULL ((void*)0)

void memset(void* dst, uint8_t value, uint32_t size);
void memcpy(void* dst, const void* src, uint32_t size);
int memcmp(const void* a, const void* b, uint32_t size);

#endif