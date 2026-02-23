#include "stdafx.h"
#include <iostream>

extern void RunAllViaviTests();

int main()
{
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    RunAllViaviTests();

    WSACleanup();

    std::cout << std::endl << "Press Enter to exit..." << std::endl;
    std::cin.get();
    return 0;
}
