#include <iostream>
#include <string>

int main() 
{
    std::string name;
    std::cout <<"What is your name\n";
    std::getline(std::cin, name);
    std::string hello_name = "Hello, " + name + "!\n";
    std::cout << hello_name;    
}