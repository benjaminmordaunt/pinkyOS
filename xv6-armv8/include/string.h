// Copyright (c) 2022 Benjamin Mordaunt
//
//     string.h
//

int strlen(const char *s);
int strnlen(const char *s, uint maxlen);
char *strrchr(const char *, int);
char *strchr(const char *, int);
unsigned long int strtoul(const char *str, char **endptr, int base);
int memcmp(const void *v1, const void *v2, uint n);
void* memset(void *dst, int v, int n);
void* memmove(void *dst, const void *src, uint n);
void* memcpy(void *dst, const void *src, uint n);
void* memchr(const void *ptr, int value, uint n);

