#ifndef HELLO_H
#define HELLO_H

#include <stdio.h>

typedef void (*callback_t)(const char *);

int initHelloWorld(callback_t callback);
void concatenate(const char *string1, const char *string2);

#endif
