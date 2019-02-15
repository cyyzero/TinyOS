#ifndef __KERNEL_STRING_H
#define __KERNEL_STRING_H

#include "stdint.h"

#define NULL ((void*)0)

void memset(void* dst, uint8_t value, uint32_t size);
void memcpy(void* dst, const void* src, uint32_t size);
int memcmp(const void* a, const void* b, uint32_t size);

char* strcpy(char* dst, const char* src);
uint32_t strlen(const char* str);
int strcmp(const char* a, const char* b);
char* strchr(const char* str, char ch);
char* strrchr(const char* str, char ch);
char* strcat(char* dst, const char* src);
uint32_t strchrs(const char* str, char ch);

#endif