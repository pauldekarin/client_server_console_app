#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cmath>
#include <vector>
#include <unistd.h>
#include <netdb.h>
#include <algorithm>
#include <cstring>
#include <termios.h>
#include <iterator>
#include <csignal>
#include <iomanip>
#include <map>
#include <fcntl.h>
#include "globals.hpp"

#define BUFFER_SIZE 8

class TCPClient{
public:
    enum Status{
        Connected,
        Disconnected
    };

    TCPClient();
    ~TCPClient();

    bool connect_to_server(char const *addr, char const *port);
    void non_blocking_loop();

    Status disconnect();

private:
    bool ping();
    bool get_uid();

    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    TCPData tcp_request;
    TCPData tcp_response;

    Status status;
    int master_socket;
    int uid;
    int chat_fd;
    
    ssize_t get_response();
    bool handle_server_response();

    bool parse_response_handler();
    bool count_response_handler();
    bool chat_response_handler();
    bool uid_response_handler();
    bool disconnected_response_handler();
};
#endif
