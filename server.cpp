#include "server.hpp"

template<class... Args>
void console_log(Args... args){
    std::cout << "[LOG] ";
    (std::cout << ... << args);
    std::cout << std::endl;
}
template<class... Args>
void console_error(Args... args){
    std::cout << "[ERROR] ";
    (std::cout << ... << args);
    std::cout << std::endl;
}

std::string get_ip_addr(int sockfd){
    static std::ostringstream stream;
    sockaddr_in s_addr;
    socklen_t s_addrlen;
    if(getpeername(sockfd, (sockaddr*)&s_addr, &s_addrlen) == 0){
        stream.str(""); 
        stream.clear();
        stream << inet_ntoa(s_addr.sin_addr)
               << ":"
               << ntohs(s_addr.sin_port);
    }

    return stream.str();
}

void set_console_echo(bool enable){
    struct termios tm;
    tcgetattr(fileno(stdin), &tm);
    tm.c_lflag &= (enable ? ECHO : ~ECHO);
    tcsetattr(fileno(stdin), TCSANOW, &tm);
}
static volatile std::sig_atomic_t sig_flag ;

void set_sig_flag(int signal){
    sig_flag = signal;
}

TCPServer::TCPServer(int port):
    port(port), 
    status(Down),
    tcp_request(TCPData()),
    tcp_response(TCPData()){
};
TCPServer::~TCPServer(){
    stop();
}

bool TCPServer::pong(int sockfd){
    console_log("Ping ", get_ip_addr(sockfd));
    tcp_response.set_data_type(DataTypes::Pong);
    return tcp_response.send_data(sockfd) > 0;
}
bool TCPServer::parse_request_handler(int sockfd){
    std::string msg{tcp_request.get(), tcp_request.size() - sizeof(DataType)};
    size_t msg_size = msg.length();
    console_log("Parse", " -- \"", msg,"\"");

    std::map<char, int> letters_map;
    for(char &ch : msg){
        letters_map[ch]++;
    }

    tcp_response.clear();
    tcp_response.set_data_type(DataTypes::Parse);
    tcp_response.append(&msg_size,sizeof(msg_size));
    tcp_response.append((void*)(msg.c_str()), msg_size);
    for(const std::pair<char,int> &p : letters_map){
        tcp_response.append((void*)(&p.first), sizeof(p.first));
        tcp_response.append((void*)(&p.second), sizeof(p.second));
    }
    return tcp_response.send_data(sockfd) > 0;
}
bool TCPServer::count_request_handler(int sockfd){
    console_log("Request Connected Clients Count ", get_ip_addr(sockfd));
    tcp_response.clear();
    tcp_response.set_data_type(DataTypes::Count);
    size_t sz = connected_clients.size();
    tcp_response.append(&sz, sizeof(sz));
    return tcp_response.send_data(sockfd) > 0;
}
bool TCPServer::chat_request_handler(int sockfd){
    int recipient_fd;
    std::memcpy(&recipient_fd, tcp_request.get(),sizeof(recipient_fd));
    console_log("Chat ", get_ip_addr(sockfd), " --> ", get_ip_addr(recipient_fd));
    tcp_response.clear();
    tcp_response.set_data_type(DataTypes::Chat);

    if(recipient_fd == 0){ // Запрос со стороны клиента на список доступных клиентов 
        
        sockaddr_in s_addr;
        socklen_t s_addrlen = sizeof(s_addr);
        size_t offset = sizeof(char) + sizeof(sockfd);
        tcp_response.append(&recipient_fd, sizeof(recipient_fd));
        for(int client_fd : this->connected_clients){
            if(client_fd == sockfd) continue;
            if(getpeername(client_fd, (sockaddr*)&s_addr, &s_addrlen) == 0){
                tcp_response.append((void*)&s_addr.sin_addr, sizeof(s_addr.sin_addr));
                tcp_response.append((void*)&s_addr.sin_port, sizeof(s_addr.sin_port));
                tcp_response.append((void*)&client_fd, sizeof(client_fd));
            }
        }
        recipient_fd = sockfd;
    }else{ // Сообщение в чате со стороны клиента к клиенту "sfd"
        if(std::find(connected_clients.begin(), connected_clients.end(), recipient_fd) != connected_clients.end()){
            tcp_response.append(&sockfd, sizeof(sockfd));
            tcp_response.append(tcp_request.get() + 4, tcp_request.size() - 5);
        }else{
            return false;
        }
    }
    return tcp_response.send_data(recipient_fd) > 0;
}
bool TCPServer::uid_request_handler(int sockfd){
    console_log("Request UID ", get_ip_addr(sockfd));
    tcp_response.clear();
    tcp_response.set_data_type(DataTypes::UID);
    tcp_response.append(&sockfd, sizeof(sockfd));
    return tcp_response.send_data(sockfd) > 0;
}

int TCPServer::start(void){
    int opt = 1;
    this->master_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(this->master_socket == -1){
        std::cerr << "Can't create socket for server!" << std::endl;
        return -1;
    }
    if(setsockopt(this->master_socket,SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        return -1;
    }
    
    sockaddr_in master_addr;
    master_addr.sin_family = AF_INET;
    master_addr.sin_port = htons(this->port);
    inet_pton(AF_INET, "127.0.0.1", &master_addr.sin_addr);

    if(bind(this->master_socket, (sockaddr*)&master_addr, sizeof(master_addr)) == -1){
        console_error("bind()");
        return -1;
    }
    if(listen(master_socket, SOMAXCONN) == -1){
        console_error("socket()");
        return -1;
    }
    console_log("Run Server At: ", inet_ntoa(master_addr.sin_addr),":",ntohs(master_addr.sin_port));
    
    this->status = Up;

    return 1;
}
void TCPServer::non_blocking_loop(){
    fd_set fds;
    int fd_socket;

    int client_socket;
    sockaddr_in client_addr;
    socklen_t client_addrlen;

    set_console_echo(false);

    while(sig_flag != SIGINT){
        FD_ZERO(&fds);
        FD_SET(this->master_socket, &fds);

        fd_socket = this->master_socket;
        for(int sockfd : this->connected_clients){
            FD_SET(sockfd, &fds);
            if(sockfd > fd_socket) fd_socket = sockfd;
        }

        int rval = select(fd_socket + 1, &fds, NULL, NULL, 0);

        if(rval == -1){
            console_error(std::strerror(errno));
            break;
        }else if(rval == 0){
            break;
        }
        
        if(FD_ISSET(this->master_socket, &fds)){
            std::memset(&client_addr, 0, sizeof(client_addr));
            if((client_socket =  this->handle_new_client(&client_addr, &client_addrlen)) < 0){
                break;
            }
            console_log("New Connection: ", get_ip_addr(client_socket));
        }else{
            for(int sockfd : this->connected_clients){
                if(FD_ISSET(sockfd, &fds)){
                    this->handle_client_message(sockfd);
                }
            }
        }

    }
    set_console_echo(true);
}
int TCPServer::stop(){
    if(this->status == Down) return 0;
    close(this->master_socket);
    connected_clients.clear();

    this->master_socket = -1;
    this->status = Down;

    console_log("Stop Server");
    return 1;
}
int TCPServer::handle_new_client(sockaddr_in *client_addr, socklen_t *client_addrlen){
    int client_socket;
    if((client_socket = accept(this->master_socket, (sockaddr*)client_addr, client_addrlen)) != -1){
        this->connected_clients.push_back(client_socket);
        pong(client_socket);
    }
    return client_socket;
}
int TCPServer::handle_client_message(int sockfd){
    ssize_t bytes_received = tcp_request.get_data(sockfd);
    DataType dtype;

    delimiter(16);
    console_log("Client: ", get_ip_addr(sockfd));
    console_log("Bytes Received: ", bytes_received);

    if(bytes_received == -1){

    }else if(bytes_received == 0){
        std::vector<int>::iterator it = std::find(
            this->connected_clients.begin(),
            this->connected_clients.end(),
            sockfd
        );
        if(it != this->connected_clients.end()){
            connected_clients.erase(it);

            tcp_request.clear();
            tcp_request.set_data_type(DataTypes::Disconnected);
            tcp_request.append((void*)&sockfd, sizeof(sockfd));

            for(int fd: connected_clients){
                tcp_request.send_data(fd);
            }

            console_log("Client Disconnected: ", get_ip_addr(sockfd));
            close(sockfd);
        }
    }else{
        switch(tcp_request.get_data_type()){
            case DataTypes::Ping:
                console_log("Ping ", get_ip_addr(sockfd));
                pong(sockfd);
                break;
            case DataTypes::Parse:
                parse_request_handler(sockfd);
                break;
            case DataTypes::Count:
                count_request_handler(sockfd);
                break;
            case DataTypes::Chat:
                chat_request_handler(sockfd);
                break;
            case DataTypes::UID:
                uid_request_handler(sockfd);
                break;
        }
    }
    return 1;
}


int main(){
    std::signal(SIGINT, set_sig_flag);

    TCPServer tcp_server = TCPServer(PORT);
    if(tcp_server.start()){
        tcp_server.non_blocking_loop();
    }
    return tcp_server.stop();
}