// string.h
#ifndef __STRING_H__
#define __STRING_H__

#include <stddef.h> // 提供 size_t 类型的定义

// 内存操作
void *memset(void *dst, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
int memcmp(const void *v1, const void *v2, size_t n);

// 字符串操作
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
char *strchr(const char *s, int c);
size_t strcspn(const char *s1, const char *s2);
size_t strspn(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);

#endif