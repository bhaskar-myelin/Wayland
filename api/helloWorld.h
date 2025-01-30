#ifndef HELLO_WORLD_H
#define HELLO_WORLD_H

// Function prototype for callback
void (*globalCallback)(int);

// Function to initialize the callback
int initHelloWorld(void (*callback)(int));

// Function to add two numbers
void addNumbers(int num1, int num2);

#endif // HELLO_WORLD_H
