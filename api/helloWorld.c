#include "helloWorld.h"

void (*globalCallback)(int);

int initHelloWorld(void (*callback)(int))
{
    globalCallback = callback;
    return 0;
}

void addNumbers(int num1, int num2)
{
    int result = num1 + num2;
    globalCallback(result);
    result = result + 10; // Adding 10 to the result
    globalCallback(result);

    return;
}
