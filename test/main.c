#include "hello.h"

void display(const char *message)
{
    printf("Callback received: %s\n", message);
}

int main()
{
    initHelloWorld(display);
    concatenate("My, ", "NAame!");
    return 0;
}
