#include <stdio.h>
#include "helloWorld.h"

// Sample callback function to print the result
void printResult(int result)
{
    printf("Result: %d\n", result);
}

int main()
{
    // Initialize the callback function
    initHelloWorld(printResult);

    // Add two numbers and process with the callback
    addNumbers(5, 3);

    return 0;
}
