#include <cstdint>
#include <iostream>
#include <cstring>
#include <ostream>
#include <string>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 65000
#define MSG_SIZE 1024

void receiveMessage(int fd){
    char buffer[MSG_SIZE];
    for (;;){
        int bytes = recv(fd, buffer, sizeof(buffer), 0);
        if (bytes == 0) continue; 
        buffer[bytes] = '\0';
 
        std::cout << "\r" << buffer << "\n";
        std::cout << "> " << std::flush;
    }
}

int main(){
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);   

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    //inet_pton(AF_INET, "36.50.55.225", &serverAddress.sin_addr.s_addr);

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        std::cout << "Connection failed!\n";
        return 1;
    }
    std::cout << "Connection succeeded!\n";

    auto sendMsg = [clientSocket](std::string message){
        if (message.size() > MSG_SIZE - 4 || message.size() < 1){
            std::cout << "Error: Cannot send message of that size.\n";
            return -1;
        }

        uint16_t msgLen = message.size();
        uint8_t c1 = (msgLen >> 8) & 0xFF;
        uint8_t c2 = msgLen & 0xFF;
        std::string packet;
        packet.push_back('1');
        packet.push_back(c1);
        packet.push_back(c2);
        packet += message;
        
        send(clientSocket, packet.c_str(), packet.size(), 0);
        return 0;
    };
    
    std::cout << "Note: While prompting, you may lose the current input due to new messages coming in. In that case, keep typing. The message isn't lost in memory, just invisible.\n";

    std::thread t(receiveMessage, clientSocket);

    std::cout << "> ";
    std::string message;
    while (std::getline(std::cin, message)){
        sendMsg(message);
        std::cout << "> ";
    }

    close(clientSocket);
    t.join();
}
