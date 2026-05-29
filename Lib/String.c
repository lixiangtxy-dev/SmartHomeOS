// string.c
#include "string.h"

void *memset(void *dst, int c, size_t n) {
    char *cdst = (char *)dst;
    for (size_t i = 0; i < n; i++) {
        cdst[i] = c;
    }
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
    char *cdst = (char *)dst;
    const char *csrc = (const char *)src;
    for (size_t i = 0; i < n; i++) {
        cdst[i] = csrc[i];
    }
    return dst;
}

int memcmp(const void *v1, const void *v2, size_t n) {
    const unsigned char *s1 = (const unsigned char *)v1;
    const unsigned char *s2 = (const unsigned char *)v2;
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        }
    }
    return 0;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == c) return (char *)s;
        s++;
    }
    if (c == '\0') return (char *)s;
    return NULL;
}

// LittleFS 解析路径时需要用到下面这两个函数
size_t strcspn(const char *s1, const char *s2) {
    size_t ret = 0;
    while (*s1) {
        if (strchr(s2, *s1)) return ret;
        s1++;
        ret++;
    }
    return ret;
}

size_t strspn(const char *s1, const char *s2) {
    size_t ret = 0;
    while (*s1) {
        if (!strchr(s2, *s1)) return ret;
        s1++;
        ret++;
    }
    return ret;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    // 逐个字符拷贝，直到遇到 '\0' 结束符
    while ((*d++ = *src++) != '\0') {
        // 循环体为空，操作都在判断条件里完成了
    }
    return dest;
}

// lib/string.c 中新增:

// 字符串拼接
char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++; // 找到 dest 的末尾
    while ((*d++ = *src++) != '\0'); // 追加 src
    return dest;
}

// 整数转字符串 (Integer to ASCII)
void itoa(int n, char s[]) {
    int i = 0, sign;
    if ((sign = n) < 0) n = -n; // 记录符号并转正
    
    // 从低位到高位提取数字
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    
    if (sign < 0) s[i++] = '-';
    s[i] = '\0';
    
    // 反转字符串
    for (int j = 0, k = i - 1; j < k; j++, k--) {
        char temp = s[j];
        s[j] = s[k];
        s[k] = temp;
    }
}