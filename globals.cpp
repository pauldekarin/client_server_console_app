#include "globals.hpp"



TCPData::TCPData(void *data_ptr, size_t data_size):data_size(data_size),data_ptr(data_ptr){}
TCPData::TCPData(const TCPData &other):data_size(other.data_size), data_ptr(std::malloc(other.data_size)){std::memcpy(data_ptr, other.data_ptr, data_size);}
TCPData::TCPData(TCPData &&other):data_size(other.data_size),data_ptr(other.data_ptr){other.data_ptr = nullptr;}
TCPData::~TCPData(){if(data_ptr != nullptr) free(data_ptr); data_ptr = nullptr;}

void TCPData::clear(){
    if(data_ptr != nullptr) free(data_ptr);
    data_ptr = nullptr;
    data_size = 0;
}
void TCPData::append(void *data, size_t size){
    if(data_ptr == nullptr){
        data_ptr = std::malloc(size);
    }else{
        data_ptr = std::realloc(data_ptr, size + data_size);
    }
    std::memcpy(static_cast<char*>(data_ptr) + data_size, data, size);
    data_size += size;
}
DataType TCPData::get_data_type(){
    if(data_ptr == nullptr) return DataTypes::Undefined;
    DataType d_type;
    std::memcpy(&d_type, data_ptr, sizeof(DataType));
    return d_type;
}
void TCPData::set_data_type(const DataType &d_type){
    if(data_ptr != nullptr){ std::memcpy(data_ptr, &d_type, sizeof(DataType)); }
    else{
        append((void*)&d_type, sizeof(DataType));
    }
}

ssize_t TCPData::send_data(int sockfd){
    if(data_ptr == nullptr) return 0;
    char *buffer = reinterpret_cast<char*>(data_ptr);
    ssize_t sended_bytes = 0;
    
    for(;sended_bytes < data_size;){
        ssize_t s_bytes = send(sockfd, buffer + sended_bytes, std::min(width, data_size - sended_bytes), 0);
        if(s_bytes <= 0){
            break;
        }
        sended_bytes += s_bytes;
    }
    return sended_bytes;
}
ssize_t TCPData::get_data(int sockfd){
    clear();
    ssize_t bytes_received = 0;
    char buffer[width];
    for(;;){
        ssize_t r_bytes = recv(sockfd, buffer, width, MSG_DONTWAIT);
        if(r_bytes <= 0){
            break;
        }
        append(buffer, r_bytes);
        bytes_received += r_bytes;
    }
    return bytes_received;
}