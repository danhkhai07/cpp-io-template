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
        if (bytes <= 0) return; 
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

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        std::cout << "Connection failed!\n";
        return 1;
    }
    std::cout << "Connection succeeded!\n";

    auto sendMsg = [clientSocket](std::string message){
        if (message.size() > MSG_SIZE - 4|| message.size() < 1){
            std::cout << "Error: Cannot send message of that size.\n";
            return -1;
        }

        size_t msgLen = message.size();
        std::string zeros = "0000";
        while (msgLen > 0){
            msgLen /= 10;
            zeros.pop_back();
        }
        message = zeros + std::to_string(message.size());
        
        send(clientSocket, message.c_str(), message.size(), 0);
        return 0;
    };
    
    // prompting name
    std::string name;
    {
        while (1){
            std::cout << "Choose your nick name: ";
            std::getline(std::cin, name);
            
            if (name.size() < 3 || name.size() > 16){
                std::cout << "Your nick name must be in between 3 and 16 characters.\n";
                continue;
            }

            bool flag = true;
            for (char c:name){
                if (!((c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or (c >= '0' and c <= 9) 
                        or c == '.' or c == '-' or c == '_')){
                    std::cout << "Your nick name cannot contain special characters.\n";
                    break;
                }
            }
            if (!flag) continue;
            sendMsg(name);
            break;
        }
    }

    std::cout << "\nNote: While prompting, you may lose the current input due to new messages coming in. In that case, keep typing. The message isn't lost in memory, just invisible.\n\n";

    std::thread t(receiveMessage, clientSocket);

    std::cout << "> ";
    std::string message;
    while (std::getline(std::cin, message)){
        sendMsg(message);
        std::cout << "\033[A\033[K> ";
    }

    close(clientSocket);
    t.join();
}
