#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

void start_client() {
    int clientSocket;
    struct sockaddr_in serverAddr;

    // 创建套接字
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Socket 创建失败!" << std::endl;
        return;
    }

    // 配置服务器地址
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // 连接到本地地址
    serverAddr.sin_port = htons(12345);  // 使用端口 12345

    // 连接到服务器
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "连接失败!" << std::endl;
        close(clientSocket);
        return;
    }
    std::cout << "成功连接到服务器!" << std::endl;
    std::string message;
    while(1)
    {
        std::getline(std::cin, message);
        // 发送数据
        //const char* message = "你好，服务器！";
        send(clientSocket, message.c_str(), message.length(), 0);
        

         // 接收数据
        char buffer[1024];
        int recvSize = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (recvSize > 0) {
            buffer[recvSize] = '\0';  // 添加 null 结尾
            std::cout << "服务器回应: " << buffer << std::endl;
        }
        
    }

   
  

    // 关闭连接
    close(clientSocket);
}


int main() {
    const char *ip = "192.168.1.1";
    struct in_addr addr;

    // 将 IPv4 地址字符串转换为二进制格式
    if (inet_pton(AF_INET, ip, &addr) == 1) {
        printf("地址转换成功，二进制地址：%u\n", addr.s_addr);
    } else {
        printf("地址转换失败\n");
    }
    std::cout<<&addr.s_addr<<std::endl;
    start_client();
    return 0;
}

/*
使用const char *massage = "你好，服务器！";
send(clientSocket, message, strlen(message), 0);
recv(clientSocket, buffer, sizeof(buffer), 0);
就可以避免内容被截断

*/