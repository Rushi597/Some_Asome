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

/**
 * @brief Starts the server.
 *
 * This function initializes the server, binds it to a specific port,
 * and listens for incoming client connections. Each client connection
 * is handled in a separate thread.
 */
void startServer();

#endif // SERVER_H
