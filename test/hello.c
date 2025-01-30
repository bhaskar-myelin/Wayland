#include "hello.h"
#include <string.h>

int initHelloWorld(callback_t callback)
{
    const char *message = "Message send from api HELLO.c";
    callback(message);
    return 0;
}

void concatenate(const char *string1, const char *string2)
{
    char result[256]; // Allocate a buffer
    snprintf(result, sizeof(result), "%s%s", string1, string2);
    printf("Concatenated String: %s\n", result);
}
