#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main(){
    //std::cout << "Hello world\n";
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);   

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = inet_addr("36.50.55.225");

    connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    const char* message = "Hi server!";
    send(clientSocket, message, strlen(message), 0);

    close(clientSocket);

}
