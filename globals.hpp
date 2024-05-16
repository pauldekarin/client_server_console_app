#ifndef GLOBALS_H
#define GLOBALS_H

#include <iostream>
#include <cstring>
#include <algorithm>
#include <sys/socket.h>

inline void delimiter(int c){
    for(int i = 0; i < c; i++) std::cout << "_";
    std::cout << std::endl;
}
typedef char DataType;

enum  DataTypes : DataType{
    Ping = 0,
    Pong = 0,
    Parse,
    Count,
    Chat,
    Disconnected,
    UID,
    Undefined
};

struct TCPData{
    TCPData() = default;
    TCPData(void *data_ptr, size_t data_size);
    TCPData(const TCPData &other);
    TCPData(TCPData &&other);
    ~TCPData();

    inline char* get(){return reinterpret_cast<char*>(data_ptr) + sizeof(DataType);}
    inline const size_t size(){return data_size;}
    void clear();
    void append(void *data, size_t size);
    DataType get_data_type();
    void set_data_type(const DataType &d_type);
    ssize_t send_data(int sockfd);
    ssize_t get_data(int sockfd);

private:
    size_t data_size = 0;
    size_t width = 4096;
    void *data_ptr = nullptr;
};

#endif