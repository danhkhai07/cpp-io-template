#include <iostream>
#include <cstring>
#include <ostream>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

void receiveMessage(int fd){
    char buffer[1024];
    for (;;){
        int bytes = recv(fd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) return;
        std::cout << "\r" << buffer << "\n" << std::flush;
    }
}

int main(){
    std::cout << "Note: Send 'EOC' to end conversation.\n";
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);   

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(1234);
    serverAddress.sin_addr.s_addr = inet_addr("36.50.55.225");

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress))){
        std::cout << "Connection failed!\n";
        return 1;
    }

    std::thread t(receiveMessage, clientSocket);

    std::string message;
    while (std::getline(std::cin, message)){
        send(clientSocket, message.c_str(), message.size() + 1, 0);
        std::cout << "> " << std::flush;
    }

    close(clientSocket);
    t.join();
}
