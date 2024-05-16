#include "client.hpp"

static const char usage_help[] = 
            "\n---Usage---\n"
            ">> parse() - Парсинг сообщения от клиента и возврат сообщения, содержащего табличку с количеством различных букв\n"
            ">> count() - Возврат количества текущих подключений по запросу клиента\n"
            ">>  chat() - Начать общение с клиентом через сервер\n"
            ">>  back() - Выйти из активного чата\n"
            ">>    id() - Отобразить свой идентификатор определенный на сервере\n"
            ">>  help() - Список команд\n"
            ">> exit() - Закрыть приложение\n"
            "---Usage---\n";

static volatile std::sig_atomic_t sig_flag ;

void set_sig_flag(int signal){
    sig_flag = signal;
}

TCPClient::TCPClient():status(Disconnected), chat_fd(0), tcp_request(TCPData()), tcp_response(TCPData()){
}
TCPClient::~TCPClient(){
    this->disconnect();
    std::cout << "Destroy..." << std::endl;
}
TCPClient::Status TCPClient::disconnect(){
    if(this->status == Disconnected) return this->status;
    std::cout << "Disconnect..." << std::endl;
    shutdown(master_socket, SHUT_WR);
    close(master_socket);
    this->status = Disconnected;
    return this->status;
}

bool TCPClient::get_uid(){
    tcp_request.clear();
    tcp_request.set_data_type(DataTypes::UID);
    tcp_request.send_data(master_socket);
    return true;
}
bool TCPClient::ping(){
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(master_socket, &fds);

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    unsigned char byte = 0;
    send(master_socket, &byte, sizeof(byte), 0);

    int rval = select(master_socket + 1, &fds, NULL, NULL, &tv);

    if(rval > 0){
        std::memset(response, 0, BUFFER_SIZE);
        int bytes_received = recv(master_socket, response, BUFFER_SIZE, 0);
        return true;
    }
    return false;
}
bool TCPClient::parse_response_handler(){
    char *p = tcp_response.get();
    size_t msg_size;
    std::memcpy(&msg_size, p,sizeof(msg_size));
    size_t offset = sizeof(msg_size);
    std::string msg{p + offset, msg_size};
    delimiter(10 + msg_size);
    std::cout << "|Message|" << msg << "|" << std::endl;
    offset+=msg_size;
    for(;offset < tcp_response.size() - 1;){
        std::pair<char,int> l_pair;
        std::memcpy(&l_pair.first, p + offset, sizeof(l_pair.first));
        offset+=sizeof(l_pair.first);
        std::memcpy(&l_pair.second,p + offset,sizeof(l_pair.second));
        offset+=sizeof(l_pair.second);
        std::cout 
            << "|" << (l_pair.first == ' ' ? '_' : l_pair.first) << std::setw(7) 
            << " |" << l_pair.second << std::setw(msg_size) 
            <<  "|" << std::endl;

    }
    delimiter(10 + msg_size);
    std::cout << std::endl;
    return true;
}
bool TCPClient::count_response_handler(){
    size_t clients_count;
    std::memcpy(&clients_count, tcp_response.get(), sizeof(clients_count));
    std::cout << "Количество подключенных клиентов: " << clients_count << std::endl;;
    return true;
}
bool TCPClient::chat_response_handler(){
    int sender_fd;
    std::memcpy(&sender_fd, tcp_response.get(), sizeof(sender_fd));
    if(sender_fd == 0){
        if(tcp_response.size() <= 5){
            std::cout << "Не с кем пообщаться..." << std::endl;
        }else{
            std::cout << "Введите ID" << std::endl;

            std::vector<int> avaliable_guests;
            in_addr ip_addr;
            in_port_t port_addr;

            for(char *p_resp = tcp_response.get() + sizeof(sender_fd); p_resp - tcp_response.get() < tcp_response.size() - 1;){
                std::memcpy(&ip_addr, p_resp, sizeof(ip_addr));
                p_resp += sizeof(ip_addr);
                std::memcpy(&port_addr, p_resp, sizeof(port_addr));
                p_resp += sizeof(port_addr);
                std::memcpy(&sender_fd, p_resp, sizeof(sender_fd));
                p_resp += sizeof(sender_fd);

                avaliable_guests.push_back(sender_fd);
                
                std::cout << "ID: " << sender_fd << "\tIP: " << inet_ntoa(ip_addr) << ":" << ntohs(port_addr) << std::endl;
            }
            std::cout << "ID: ";
            std::cin >> sender_fd;
            if(std::cin.good() && std::find(avaliable_guests.begin(), avaliable_guests.end(), sender_fd) != avaliable_guests.end()){
                chat_fd = sender_fd;
            }else{
                std::cout << "Unavaliable ID" << std::endl;
            }
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        
    }else if(chat_fd == sender_fd){
        std::cout 
            << "\r[Chat]Отправитель " << sender_fd << ": "  
            << reinterpret_cast<char*>(tcp_response.get()) + sizeof(sender_fd) 
            << std::endl;
    }
    return true;
}
bool TCPClient::uid_response_handler(){
    std::cout << "Byte " << tcp_response.size() << std::endl;
    std::memcpy(&uid, tcp_response.get(), sizeof(uid));
    return true;
}
bool TCPClient::disconnected_response_handler(){
    int disconnected_fd;
    std::memcpy(&disconnected_fd, tcp_response.get(), sizeof(disconnected_fd));
    if(chat_fd == disconnected_fd){
        std::cout << "\r[Chat]Чат закрыт с Клиентом: " << chat_fd << std::endl;
        chat_fd = 0;
    }
    std::cout << "\r[Disconnect]Клиент: " << disconnected_fd << std::endl;
    return true;
}

bool TCPClient::handle_server_response(){
    ssize_t bytes_received = tcp_response.get_data(master_socket);
    if(bytes_received == -1){
        std::cerr << std::strerror(errno) << std::endl;
    }else if(bytes_received == 0){
        std::cerr << "Timeout";
        return false;
    }else{
        switch(tcp_response.get_data_type()){
            case DataTypes::Parse:
                parse_response_handler();
                break;
            case DataTypes::Count:
                count_response_handler();
                break;
            case DataTypes::Chat:
                chat_response_handler();
                break;
            case DataTypes::UID:
                uid_response_handler();
                break;
            case DataTypes::Disconnected:
                disconnected_response_handler();
                break;
        }
    }
    return true;
}
void TCPClient::non_blocking_loop(){
    get_uid();
    struct timeval tv;

    while(sig_flag != SIGINT){
        if(chat_fd > 0){
            std::cout << "[Chat " << chat_fd << "] " << std::flush;
        }
        fd_set fds;
        
        FD_ZERO(&fds);
        FD_SET(fileno(stdin), &fds);
        FD_SET(this->master_socket, &fds);

        tv.tv_sec = 0;
        tv.tv_usec = 50;
        int rval = select(std::max(fileno(stdin), this->master_socket) + 1, &fds, NULL, NULL, NULL);

        if(rval < 0){ 
            break;
        }else if(rval == 0){ 

        }else{ 
            if(FD_ISSET(fileno(stdin), &fds)){
                std::string line;
                if(!std::getline(std::cin, line)){ continue; }
                if(line.compare("exit()") == 0){ break; }
                if(line.compare("help()") == 0) { std::cout << usage_help << std::endl; }
                else if(line.compare("back()") == 0) {chat_fd = 0;}
                else if(chat_fd){
                    tcp_request.clear();
                    tcp_request.set_data_type(DataTypes::Chat);
                    tcp_request.append((void*)&(chat_fd), sizeof(chat_fd));
                    tcp_request.append((void*)(line.c_str()), line.length());
                    tcp_request.send_data(master_socket);
                }else if(line.compare("parse()") == 0){
                    std::cout << "Спарсить Сообщение: ";
                    std::getline(std::cin, line);

                    tcp_request.clear();
                    tcp_request.set_data_type(DataTypes::Parse);
                    tcp_request.append((void*)line.c_str(), line.length());
                    tcp_request.send_data(master_socket);
                }else if(line.compare("count()") == 0){
                    tcp_request.clear();
                    tcp_request.set_data_type(DataTypes::Count);
                    tcp_request.send_data(master_socket);
                }else if(line.compare("chat()") == 0){
                    tcp_request.clear();
                    tcp_request.set_data_type(DataTypes::Chat);
                    int i_zero_byte = 0;
                    tcp_request.append((void*)&i_zero_byte,sizeof(i_zero_byte));
                    tcp_request.send_data(master_socket);
                }else if(line.compare("id()") == 0){
                    std::cout << "Твой ID: " << this->uid << std::endl;
                }else{
                    std::cout << "Вспомогательная функция help() -- список команд" << std::endl;
                }
            }else if(FD_ISSET(this->master_socket, &fds)){
                if(this->handle_server_response() == 0){ break; }
            }else{

            }
        }   
    }
}
bool TCPClient::connect_to_server(char const *host, char const *port){
    std::cout << "Trying connect to: " << host << ":" << port << std::endl;
    for(int i_tries = 1; i_tries < 8 && status == Disconnected; i_tries++){
        std::cout << "Attempt #" <<  i_tries << std::endl;
        if((master_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            continue;
        }
        sockaddr_in serv_sock_addr;
        serv_sock_addr.sin_family = AF_INET;
        serv_sock_addr.sin_port = htons(atoi(port));
        if(inet_pton(AF_INET, host, &serv_sock_addr.sin_addr) < 0){
            continue;
        }
        int fl = fcntl(master_socket, F_GETFL);
        fl |= O_NONBLOCK;
        fcntl(master_socket, F_SETFL, fl);
        int res = connect(master_socket, (struct sockaddr*)&serv_sock_addr, sizeof(serv_sock_addr));
        if(res < 0){
            if(errno == EINPROGRESS){
                while(true){
                    fd_set fds;
                    FD_ZERO(&fds);  
                    FD_SET(master_socket, &fds);

                    struct timeval tv;
                    tv.tv_sec = 0;
                    tv.tv_usec = 500;

                    int r = select(master_socket + 1,&fds, NULL, NULL, &tv);
                    if(r < 0 && errno != EINTR){
                        std::cout << std::strerror(errno) << std::endl;
                    }else if(r == 0){
                        std::cout << "Timeout" << std::endl;
                    }
                    else{
                        int err;
                        socklen_t err_len = sizeof(err);
                        int r = getsockopt(master_socket,SOL_SOCKET, SO_ERROR,&err,&err_len);
                        if(err == 0){
                            status = Connected;
                            break;
                        }
                    }      
                    close(master_socket);
                    break;
                }
            }
        }
        
        fl &= ~O_NONBLOCK;
        fcntl(master_socket, F_SETFL, fl);
    }
    std::cout << std::endl;
    return this->status == Connected;
}

int main(int argc, char **argv){
    if (argc != 3){
        std::cerr << "usage: requiest only IP and PORT" << std::endl;
        return EXIT_FAILURE;
    }

    std::signal(SIGINT, set_sig_flag);

    TCPClient client = TCPClient();
    char const *ip = argv[1];
    char const *port = argv[2];
    char buf[256];
    if(inet_pton(AF_INET, ip, buf) <= 0){
        std::cerr << "Unallowable IP " << ip << std::endl;
        return EXIT_FAILURE;
    }
    for(char *p = argv[2]; *p;p++){
        if(*p < 48 || *p > 57){
            std::cerr << "Port has to be contains only digits" << std::endl;
            return EXIT_FAILURE;
        }
    }
    if(!client.connect_to_server(ip,port)){
        std::cerr << "Unable to connect to remote host" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Successfully Connected!" << std::endl;
    std::cout << usage_help << std::endl;
    client.non_blocking_loop();
    
    return client.disconnect() == TCPClient::Status::Disconnected;
}