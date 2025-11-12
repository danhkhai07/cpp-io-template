#include <iterator>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include <cstdio>
#include <iostream>
#include <map>
#include <queue>
#include <utility>

#define MAX_CLIENTS 5
#define PORT 65000
#define MSG_SIZE 1024
#define PACKET_SIZE 16

int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

struct TCPParser{
private:
    std::queue<int>             senderQueue;
    std::queue<int>             receiverQueue;
    std::queue<char[MSG_SIZE]>  messageQueue; // { receiver , message }

        int msgLen = 0;
        size_t capturedMsgLen = 0; 

        int receiverFd = 0;
        size_t capturedReceiverFd = 0; 

        char pendingMsg[MSG_SIZE];
        size_t pendLen = 0;

    int feed(char* packet, size_t len){
        for (size_t i = 0; i < len; ++i, ++packet){
            char c = *packet;
            if (capturedReceiverFd < 4){
                if (c < '0' || c > '9') return -1;
                receiverFd = c - '0' + receiverFd*10;
                capturedReceiverFd++;
                continue;
            } 

            if (capturedMsgLen < 4){
                msgLen = c - '0' + msgLen*10;
                capturedMsgLen++;
                continue;
            } 

            pendingMsg[pendLen] = c;
            pendLen++;
            msgLen--;

            if (msgLen == 0){
                messageQueue.push(pendingMsg);
                receiverQueue.push(receiverFd);
                capturedMsgLen = 0;
                pendLen = 0;
            }
        }
        return 0;
    }

public:
    struct Message{
        int receiver, sender;
        std::string message;
    };

    int process_msg(int sender, char* buffer, size_t len){
        feed(buffer, len);
        senderQueue.push(sender);
        while (!(senderQueue.empty() || receiverQueue.empty() || messageQueue.empty())){
            Message msg; 
            msg.sender      = senderQueue.front();
            msg.receiver    = receiverQueue.front();
            msg.message     = messageQueue.front();

            senderQueue.pop();
            receiverQueue.pop();
            messageQueue.pop();
        }
        return 0;
    }
};

int main(){
    TCPParser psr;

    int newFd;
    int nfds = 1;
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;   
    socklen_t addrlen = sizeof(serverAddress);
    serverAddress.sin_family        = AF_INET;
    serverAddress.sin_port          = htons(PORT);
    serverAddress.sin_addr.s_addr   = INADDR_ANY;
        
    if (bind(serverFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        perror("Bind failed");
        return 1;
    }

    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }

    if (listen(serverFd, 5) < 0){
        perror("Listen failed");
        return 1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";
    
    int epfd = epoll_create1(0);
    struct epoll_event ev, events[MAX_CLIENTS];

    ev.events = EPOLLIN | EPOLLRDHUP;
    ev.data.fd = serverFd;
    setNonBlocking(serverFd);
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, serverFd, &ev) < 0){
        perror("epoll_ctl failed");
    }

    while (1){
        int nfds = epoll_wait(epfd, events, MAX_CLIENTS, 0);
        if (nfds == 0) continue;
        if (nfds < 0){ 
            perror("epoll_wait failed");
            break;
        }
        
        for (int i = 0; i < nfds; i++){
            if (events[i].data.fd == serverFd){
                int clientFd = accept(serverFd, nullptr, nullptr);
                ev.events = EPOLLIN;
                ev.data.fd = clientFd;
                setNonBlocking(clientFd);
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, clientFd, &ev) < 0){
                    perror("epoll_ctl failed");
                }
                continue;
            }

            if (events[i].events & EPOLLRDHUP){
                std::cout << "Client fd" << events[i].data.fd << " disconnected.\n";
                close(events[i].data.fd);
                continue;
            }
                     
            if (events[i].events & EPOLLIN){
                char buffer[PACKET_SIZE];
                int bytes = read(events[i].data.fd, buffer, sizeof(buffer));
                psr.process_msg(events[i].data.fd, buffer, sizeof(buffer));
            }
                
            if (events[i].events & EPOLLOUT){
                
            }
        }
    }

    close(serverFd);
    return 0;
}
