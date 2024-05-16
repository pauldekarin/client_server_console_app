#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <algorithm>
#include <map>
#include <iomanip>
#include <csignal>
#include <functional>
#include <termios.h>
#include "globals.hpp"

#define PORT 4040

class TCPServer{
public:
    enum Status{
        Up = 1,
        Down
    };

    TCPServer() = default;
    TCPServer(const int port);
    ~TCPServer();

    int start();
    void non_blocking_loop();
    int stop();
private:
    TCPData tcp_response;
    TCPData tcp_request;

    Status status;

    int master_socket;
    int port;

    std::vector<int> connected_clients;

    int handle_client_message(int);
    int handle_new_client(sockaddr_in *, socklen_t*);

    bool parse_request_handler(int);
    bool count_request_handler(int);
    bool chat_request_handler(int);
    bool disconnected_request_handler(int);
    bool uid_request_handler(int);
    bool pong(int);
};

#endif