#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

void start_server() {
    uint32_t  b=123;
    uint32_t  a=htonl(b);
    std::cout<<b<<std::endl;
    std::cout<<a<<std::endl;

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientSize = sizeof(clientAddr);

    // 创建套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Socket 创建失败!" << std::endl;
        return;
    }

    // 配置服务器地址
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;  // 监听所有接口
    serverAddr.sin_port = htons(12346);  // 使用端口 12345

    // 绑定套接字
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "绑定失败!" << std::endl;
        close(serverSocket);
        return;
    }

    // 开始监听
    if (listen(serverSocket, 1) == -1) {
        std::cerr << "监听失败!" << std::endl;
        close(serverSocket);
        return;
    }
    std::cout << "服务器已启动，监听端口 12345..." << std::endl;

    // 接受客户端连接
    clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientSize);
    if (clientSocket == -1) {
        std::cerr << "连接失败!" << std::endl;
        close(serverSocket);
        return;
    }

    std::cout << "客户端已连接!" << std::endl;

    // 接收和发送数据
    char buffer[1024];
    int recvSize;
    while ((recvSize = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[recvSize] = '\0';  // 以 null 结尾
        std::cout << "收到数据: " << buffer << std::endl;
        send(clientSocket, "收到你的消息!", 15, 0);  // 发送回应
    }

    if (recvSize == 0) {
        std::cout << "客户端关闭连接!" << std::endl;
    } else {
        std::cerr << "接收数据失败!" << std::endl;
    }

    // 关闭连接
    close(clientSocket);
    close(serverSocket);
}

int main() {
    start_server();
    return 0;
}
/*
seq 我的数据从这开始
ack 你的数据从这开始
*/