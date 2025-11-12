#include <string>
#include <iostream>

std::string input = "0015kdmcjdhfngl12jx0008judhdhsb0003bbb";

struct TCPParser {
    int msgLen = 0;
    int capturedMsgLen = 0;
    char pendingMsg[1024];
    size_t pendLen = 0;

    int splitMessages(char *packet, int len){
        for (int i = 0; i < len; ++i, ++packet){
            char c = *packet;
            if (capturedMsgLen < 4){
                msgLen = c - '0' + msgLen*10;
                capturedMsgLen++;
            } else {
                pendingMsg[pendLen] = c;
                pendLen++;
                msgLen--;

                if (msgLen == 0){
                    std::cout.write(pendingMsg, pendLen);
                    std::cout << '\n';
                    capturedMsgLen = 1;
                    pendLen = 0;
                }
            }
        }
        return 0;
    }
};

int main(){
    TCPParser psr;
    int count = 0;
    char tmp[1024];
    int tmpLen = 0;
    for (int i = 0; i < input.size(); i++){
        tmp[tmpLen] = input[i];
        tmpLen++;
        count++;
        if (count == 3 or i == input.size() - 1){
            psr.splitMessages(tmp, tmpLen);
            count = 0;
            tmpLen = 0;
        } 
    }
    return 0;
}
