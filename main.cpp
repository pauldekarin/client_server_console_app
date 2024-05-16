#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <cstring>

#define PORT 4040

int main(int argc, char **argv){
    //Create server socket
    int master_socket = socket(AF_INET,SOCK_STREAM,0);
    if(master_socket == -1){
        std::cerr << "Cant't create master socket!" << std::endl;
        return EXIT_FAILURE;
    }

    sockaddr_in s_addr;
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(PORT); // Переводим PORT из числового вида в требуемый для работы с TCP/IP 
    inet_pton(AF_INET, "127.0.0.1",&s_addr.sin_addr); // Определяем любой доступный IP-адресс для сервера

    if(bind(master_socket, (sockaddr*)&s_addr, sizeof(s_addr)) == -1){
        std::cerr << "Can't bind master socket!" << std::endl;
        return -1;
    }
    if(listen(master_socket,SOMAXCONN) == -1){
        std::cerr << "Can't listen master socket!" << std::endl;
        return -1;
    }

    char host[NI_MAXHOST];
    char svc[NI_MAXSERV];
    memset(host, 0, NI_MAXHOST);
    memset(svc, 0, NI_MAXSERV);

    if(getnameinfo((sockaddr*)&s_addr, sizeof(s_addr), host, NI_MAXHOST, svc, NI_MAXSERV,0)){
        std::cout << host << ":" << svc << std::endl;
    }else{
        inet_ntop(AF_INET, &s_addr.sin_addr, host, NI_MAXHOST);
        std::cout << "Start At: " << host << " Socket:" << master_socket << std::endl;
    }
    sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);

    int client_socket;
    if((client_socket = accept(master_socket, (sockaddr*)&client_addr, &client_addrlen)) == -1){
        std::cerr << "" << std::endl;
    };

    printf("accepted\n");
    if(getnameinfo((sockaddr*)&client_addr, client_addrlen, host, NI_MAXHOST, svc, NI_MAXSERV,0)){
        std::cout << host << "connected on" << svc << std::endl;
    }else{
        inet_ntop(AF_INET, &client_addr.sin_addr,host, NI_MAXHOST);
        std::cout << host << std::endl;
    }

    char buffer[4096];

    int index = 0;
    while(index < 2){
        index++;
        memset(buffer, 0, 4096);
        printf("recv\n");
        int bytes_received = recv(client_socket, buffer, 4096,0);
        if(bytes_received == -1){
            break;
        }else if(bytes_received == 0){
            break;
        }

        std::cout << "Received: " << buffer << std::endl;

        send(client_socket, buffer, 4096, 0);
    }
    close(client_socket);
    close(master_socket);
    return EXIT_SUCCESS;
}