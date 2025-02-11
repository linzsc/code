#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <pthread.h>

// 用于传递给线程的结构体
struct ClientData {
    int clientSocket;
    struct sockaddr_in clientAddr;
};

// 处理客户端的请求
void* handle_client(void* arg) {
    // 获取传递的客户端数据
    ClientData* data = (ClientData*)arg;
    int clientSocket = data->clientSocket;
    struct sockaddr_in clientAddr = data->clientAddr;

    // 打印客户端的 IP 地址和端口
    char clientIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
    std::cout << "客户端 IP: " << clientIp << ", 端口: " << ntohs(clientAddr.sin_port) << std::endl;

    char buffer[1024];
    int recvSize;

    // 处理客户端请求
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
    delete data;  // 释放客户端数据
    return nullptr;  // 结束线程
}

// 启动服务器，监听并接受客户端请求
void start_server() {

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
    serverAddr.sin_port = htons(12345);  // 使用端口 12345

    // 绑定套接字
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "绑定失败!" << std::endl;
        close(serverSocket);
        return;
    }

    // 开始监听
    if (listen(serverSocket, 5) == -1) {
        std::cerr << "监听失败!" << std::endl;
        close(serverSocket);
        return;
    }
    std::cout << "服务器已启动，监听端口 12345..." << std::endl;

    // 接受客户端连接
    while (true) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientSize);
        if (clientSocket == -1) {
            std::cerr << "连接失败!" << std::endl;
            close(serverSocket);
            return;
        }

        std::cout << "客户端已连接!" << std::endl;

        // 创建 ClientData 结构体并传递给线程
        ClientData* data = new ClientData;
        data->clientSocket = clientSocket;
        data->clientAddr = clientAddr;

        // 创建线程来处理客户端请求
        pthread_t threadId;
        if (pthread_create(&threadId, nullptr, handle_client, (void*)data) != 0) {
            std::cerr << "创建线程失败!" << std::endl;
            close(clientSocket);
            delete data;  // 确保删除分配的内存
            continue;
        }

        // 分离线程，自动回收线程资源
        pthread_detach(threadId);
    }

    // 关闭监听套接字（通常不会执行到这里）
    close(serverSocket);
}

int main() {
    start_server();
    return 0;
}
