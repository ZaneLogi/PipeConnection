// PipeConnection.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "demo_server.h"

int main()
{
    demo_server server;
    server.start();
    std::cout << "server starts\n";

    Sleep(60000);

    std::cout << "server stops\n";
    server.stop();
    return 0;
}

