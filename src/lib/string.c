#include "string.h"
#include "global.h"
#include "debug.h"

// 将dest起始的size个字节初始化为value
void memset(void* dst, uint8_t value, uint32_t size)
{
    ASSERT(dst != NULL);
    for (uint32_t i = 0; i < size; ++i)
    {
        ((uint8_t*)dst)[i] = value;
    }
}

// 将src起始的size个字节复制到dst
void memcpy(void* dst, const void* src, uint32_t size)
{
    ASSERT(dst != NULL && src != NULL);
    for (uint32_t i = 0; i < size; ++i)
    {
        ((uint8_t*)dst)[i] = ((const uint8_t*)src)[i];
    }
}

// 连续比较以地址a和地址b开头的size个字节
//相等返回0；a>b返回+1；a<b返回-1
int memcmp(const void* a, const void* b, uint32_t size)
{
    ASSERT(a != NULL || b != NULL);
    for (uint32_t i = 0; i < size; ++i)
    {
        char left  = ((const char*)a)[i];
        char right = ((const char*)b)[i];
        if (left > right)
            return 1;
        if (left < right)
            return -1;
    }
    return 0;
}

// 将字符串从src复制到dst
char* strcpy(char* dst, const char* src)
{
    ASSERT(dst != NULL && src != NULL);
    char* ret = dst;
    while ((*dst++ = *src++))
        continue;
    return ret;
}

// 返回字符串长度
uint32_t strlen(const char* str)
{
    ASSERT(str != NULL);
    const char* p = str;
    while (*str++)
        continue;
    return (str - p - 1);
}

// 比较两个字符串，若a中的字符串大于b中的字符串返回1，相等返回0，小于返回-1
int strcmp(const char* a, const char* b)
{
    ASSERT(a != NULL && b != NULL);
    while (*a && *a == *b)
    {
        ++a;
        ++b;
    }
    return *a < *b ? -1 : *a > *b;
}

// 从前往后查找字符串str中首个字符ch出现的地址
char* strchr(const char* str, char ch)
{
    ASSERT(str != NULL);
    while (*str)
    {
        if (ch == *str)
            return (char*)str;
        ++str;
    }
    return NULL;
}

// 从后往前字符串str中首个字符ch出现的地址
char* strrchr(const char* str, char ch)
{
    ASSERT(str != NULL);
    const char* last_char = NULL;
    while (*str)
    {
        if (*str == ch)
            last_char = str;
        ++str;
    }
    return (char*)last_char;
}

// 将src拼接到dst后，并返回拼接后串的地址
char* strcat(char* dst, const char* src)
{
    ASSERT(dst != NULL && src != NULL);
    char* str = dst;
    while (*str)
        ++str;
    while ((*str++ = *src++))
        continue;
    return dst;
}

// 在字符串中查找字符ch出现的次数
uint32_t strchrs(const char* str, char ch)
{
    ASSERT(str != NULL);
    uint32_t cnt = 0;
    while (*str)
    {
        if (*str == ch)
            ++cnt;
        ++str;
    }
    return cnt;
}

