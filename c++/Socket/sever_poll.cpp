#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

void start_server() {
    #pragma region 创建套接字
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
    
    #pragma endregion
    // 使用 poll 监听多个文件描述符
    struct pollfd fds[MAX_CLIENTS + 1];
    fds[0].fd = serverSocket;
    fds[0].events = POLLIN;
    int nfds = 1;  // 当前活跃的文件描述符数量

    while (true) {
        // 使用 poll 来监视文件描述符
        int activity = poll(fds, nfds, -1);
        if (activity < 0) {
            std::cerr << "Poll 错误!" << std::endl;
            continue;
        }

        // 检查是否有新连接
        if (fds[0].revents & POLLIN) {
            clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientSize);
            if (clientSocket == -1) {
                std::cerr << "连接失败!" << std::endl;
                continue;
            }

            // 将新客户端套接字加入到 fds 数组
            fds[nfds].fd = clientSocket;
            fds[nfds].events = POLLIN;
            nfds++;

            // 打印客户端的 IP 地址和端口
            char clientIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
            std::cout << "客户端 IP: " << clientIp << ", 端口: " << ntohs(clientAddr.sin_port) << " 已连接!" << std::endl;
        }

        // 遍历所有客户端套接字，检查是否有数据可读
        for (int i = 1; i < nfds; ++i) {
            if (fds[i].revents & POLLIN) {
                char buffer[BUFFER_SIZE];
                int recvSize = recv(fds[i].fd, buffer, sizeof(buffer), 0);
                if (recvSize > 0) {
                    buffer[recvSize] = '\0';  // 添加字符串结束符
                    std::cout << "收到来自客户端的数据: " << buffer << std::endl;
                    const char* message = "服务器收到你的消息了!";
                    // 向客户端发送回应
                    send(fds[i].fd, message, strlen(message), 0);
                } else if (recvSize == 0) {
                    std::cout << "客户端关闭连接!" << std::endl;
                    close(fds[i].fd);
                    // 从 fds 数组中移除该客户端
                    for (int j = i; j < nfds - 1; ++j) {
                        fds[j] = fds[j + 1];
                    }
                    nfds--;
                } else {
                    std::cerr << "接收数据失败!" << std::endl;
                    close(fds[i].fd);
                    // 从 fds 数组中移除该客户端
                    for (int j = i; j < nfds - 1; ++j) {
                        fds[j] = fds[j + 1];
                    }
                    nfds--;
                }
            }
        }
    }

    // 关闭服务器套接字（通常不会执行到这里）
    close(serverSocket);
}

int main() {
    start_server();
    return 0;
}
