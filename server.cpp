#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <cstring>
#include <cstdio>
#include <memory>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
#include <algorithm>

#define MAX_CLIENTS 5
#define PORT 65000
#define PACKET_SIZE 1024
#define BUF_SIZE 16

namespace container {
    class Request {
    protected:
        int destination;
        std::string raw;
    public:
        Request(int dest, std::string rw):
             destination(dest), raw(rw) {}
        virtual ~Request() = default;
        std::string getRaw(){
            return raw;
        }
    };

    class Message : public Request {
    private:
        std::string msg;
    public:
        int sender;
        std::string getOutput() {
            return msg;
        }
    };

    enum class CommandCode {
        NULL_CMD
    };

    class Command : public Request {
    private:
    public:
        CommandCode code = CommandCode::NULL_CMD;
    };
};

class PacketParser {
private:
    struct ParsingRequest {
        int lenParsed = 0;
        
    };
    int pending = 0;
    std::unordered_map<int, std::string> requests;

public:
    int feed(int sender, std::string packet){
        
    }

    bool empty(){
        return (pending == 0);
    } 

    container::Request getRequest();
};

class Server {
private:
    // TCP Parser var
    PacketParser Parser;

    // Server shit
    int clientCount = 0;
    int serverFd;
    int port;
    int epfd;
    sockaddr_in serverAddr;
    epoll_event events[MAX_CLIENTS + 1];

    // TCP queue shit
    struct ToSendMessage {
        int offset = 0;
        std::string msg;
    };
    std::queue<std::unique_ptr<container::Request>> toGetQueue;
    std::unordered_map<int, std::queue<ToSendMessage>> toSendQueue;
    
    int setNonBlocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) return -1;
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

public:
    Server(){}
    ~Server(){
        close(serverFd);
    }
    int initialize(int prt){
        serverFd = socket(AF_INET, SOCK_STREAM, 0);
        if (serverFd < 0) return -1;  

        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(prt);
        serverAddr.sin_family = INADDR_ANY;

        if (bind(serverFd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) return -1;
        if (listen(serverFd, MAX_CLIENTS) < 0) return -1;
        setNonBlocking(serverFd);

        epoll_event tmp_ev;    
        tmp_ev.events = EPOLLIN | EPOLLRDHUP;
        tmp_ev.data.fd = serverFd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, serverFd, &tmp_ev);

        std::cout << "Server initialized on port: " << PORT << "\n";
        return 0;
    }

    int process(){
        epoll_event tmp_ev;
        int nfds = epoll_wait(epfd, events, MAX_CLIENTS, 0);

        for (int i = 0; i <= nfds; ++i){
            int fd = events[i].data.fd;

            if (fd == serverFd){
                sockaddr_in clientAddr;
                socklen_t addrLen = sizeof(clientAddr);
                int clientFd = accept(serverFd, (sockaddr*)&clientAddr, &addrLen);
                std::string clientIPv4 = inet_ntoa(clientAddr.sin_addr);

                if (clientCount >= MAX_CLIENTS){
                    close(clientFd);
                    std::cout << "IP " << clientIPv4 << " tried to connect but failed due to: (probably maxed capacity)\n";
                    continue;
                }

                tmp_ev.events = EPOLLIN | EPOLLRDHUP;
                tmp_ev.data.fd = clientFd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clientFd, &tmp_ev);
                std::cout << "IP " << clientIPv4 << " connected through fd number " << clientFd << ".\n";
            }

            if (events[i].events & EPOLLIN){
                std::string buffer;
                buffer.resize(BUF_SIZE);
                size_t bytes = read(fd, buffer.data(), buffer.size());

                if (bytes <= 0) continue;
                Parser.feed(fd, buffer);
            }

            if (events[i].events & EPOLLOUT){
                ToSendMessage* buffer = &toSendQueue[fd].front();
                int messageSize = std::min((int)buffer->msg.size() - buffer->offset, BUF_SIZE);
                int sent = send(fd, buffer->msg.data() + buffer->offset, BUF_SIZE, 0);
                buffer->offset += sent;
                if (buffer->offset >= buffer->msg.size()) toSendQueue[fd].pop();
            }

            if (events[i].events & EPOLLRDHUP){
                close(fd);
                std::cout << "Client at fd number " << fd << " disconnected.\n";
            }
        }

        return 0;
    }

    //int poll();
   
    /// The queue stores unique_ptr, so you MUST utilize the result, as queue.front() is popped
    /// immediately after retrieval.
    int getRequest(std::unique_ptr<container::Request>& result){
        result = std::move(toGetQueue.front());
        toGetQueue.pop();
        return 0;
    }

    /// ONLY add the request to queue; the queue is processed in process(). 
    int sendRequest(int fd, std::string msg){
        ToSendMessage sending;
        sending.msg = msg;
        toSendQueue[fd].push(sending);
        return 0;
    }
};

int main() {
    Server Server;
    Server.initialize();
    while (1){
        Server.poll();
        if ( != nullptr){
            std::unique_ptr<Request> request(Server.get());
            request = request.process();
        }
    }

    return 0;
}

