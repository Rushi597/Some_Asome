#include "server.h"
#include <algorithm>
#include <cstring>
#include <cerrno>

std::vector<int> clients; ///< Vector to store client sockets
std::map<int, std::string> client_names; ///< Map to store client names indexed by their socket
std::mutex mtx; ///< Mutex to synchronize access to shared resources

/**
 * @brief Broadcasts a message to all connected clients except the sender.
 *
 * @param message The message to broadcast.
 * @param sender_socket The socket of the sender.
 */
void broadcastMessage(const std::string& message, int sender_socket) {
    std::lock_guard<std::mutex> lock(mtx);
    for (int client_socket : clients) {
        if (client_socket != sender_socket) {
            send(client_socket, message.c_str(), message.length(), 0);
        }
    }
}

/**
 * @brief Handles communication with a connected client.
 *
 * @param client_socket The socket of the connected client.
 */
void handleClient(int client_socket) {
    char buffer[1024];
    int valread;
    std::string message_accumulator;

    valread = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (valread <= 0) {
        #ifdef _WIN32
        closesocket(client_socket);
        #else
        close(client_socket);
        #endif
        return;
    }
    buffer[valread] = '\0';
    std::string username(buffer);
    {
        std::lock_guard<std::mutex> lock(mtx);
        clients.push_back(client_socket);
        client_names[client_socket] = username;
    }

    std::string welcome_message = "Welcome, " + username + "!\n";
    send(client_socket, welcome_message.c_str(), welcome_message.length(), 0);
    broadcastMessage(username + " has joined the chat.\n", client_socket);

    while (true) {
        valread = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (valread <= 0) {
            break;
        }
        buffer[valread] = '\0';
        message_accumulator += std::string(buffer);

        size_t pos;
        while ((pos = message_accumulator.find('\n')) != std::string::npos) {
            std::string complete_message = message_accumulator.substr(0, pos + 1);
            message_accumulator.erase(0, pos + 1);

            if (complete_message.substr(0, 4) == "/msg") {
                size_t pos = complete_message.find(' ', 5);
                if (pos != std::string::npos) {
                    std::string recipient = complete_message.substr(5, pos - 5);
                    std::string private_msg = "[Private from " + username + "]: " + complete_message.substr(pos + 1);
                    broadcastMessage(private_msg, client_socket);
                }
            } else {
                std::string broadcast_msg = username + ": " + complete_message;
                broadcastMessage(broadcast_msg, client_socket);
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
        client_names.erase(client_socket);
    }

    broadcastMessage(username + " has left the chat.\n", client_socket);
    #ifdef _WIN32
    closesocket(client_socket);
    #else
    close(client_socket);
    #endif
}


/**
 * @brief Starts the server, binds it to a port, and listens for incoming connections.
 *
 * The server runs indefinitely, accepting new client connections and
 * spawning a new thread to handle each client.
 */
void startServer() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    #ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return;
    }
    #endif

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) != 0) {
        int err = errno;
        std::cerr << "setsockopt failed: " << strerror(err) << " (errno: " << err << ")" << std::endl;
        #ifdef _WIN32
        closesocket(server_fd);
        #else
        close(server_fd);
        #endif
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        #ifdef _WIN32
        closesocket(server_fd);
        #else
        close(server_fd);
        #endif
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        #ifdef _WIN32
        closesocket(server_fd);
        #else
        close(server_fd);
        #endif
        exit(EXIT_FAILURE);
    }

    std::vector<std::thread> threads;

    while (true) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("accept failed");
            #ifdef _WIN32
            closesocket(server_fd);
            #else
            close(server_fd);
            #endif
            exit(EXIT_FAILURE);
        }

        threads.push_back(std::thread(handleClient, new_socket));
    }

    for (auto &th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    #ifdef _WIN32
    closesocket(server_fd);
    #else
    close(server_fd);
    #endif

    #ifdef _WIN32
    WSACleanup();
    #endif
}
