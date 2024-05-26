#ifndef SERVER_H
#define SERVER_H

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    typedef int socklen_t;
#else
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
#endif

#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <algorithm>

void startServer();

#endif // SERVER_H
