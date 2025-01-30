#include <iostream>
#include "helloWorld.hpp"
using namespace std;

void listenConcatenate(string concatenatedString)
{
    cout << "Concatenated string is " << concatenatedString << endl;
}
int main()
{
    initHelloWorld(listenConcatenate);
    concatenate("Hello", "world");
    return 0;
}
