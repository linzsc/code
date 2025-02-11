#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#include <signal.h>

// 处理客户端的请求
void handle_client(int clientSocket) {
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
}

// 信号处理程序，用于回收子进程
void sigchld_handler(int sig) {
    // 调用 waitpid 来回收已结束的子进程
    while (waitpid(-1, nullptr, WNOHANG) > 0) {
        // 不需要做其他处理，waitpid 返回已结束的子进程
    }
}

// 启动服务器，监听并接受客户端请求
void start_server() {
    uint32_t b = 123;
    uint32_t a = htonl(b);
    std::cout << b << std::endl;
    std::cout << a << std::endl;

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
    if (listen(serverSocket, 5) == -1) {
        std::cerr << "监听失败!" << std::endl;
        close(serverSocket);
        return;
    }
    std::cout << "服务器已启动，监听端口 12345..." << std::endl;

    // 设置信号处理程序，用于回收子进程
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;  // 设置信号处理程序
    sigemptyset(&sa.sa_mask);  // 清空信号掩码
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;  // 不阻塞中断
    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
        std::cerr << "设置 SIGCHLD 处理程序失败!" << std::endl;
        close(serverSocket);
        return;
    }

    // 接受客户端连接
    while (true) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientSize);
        if (clientSocket == -1) {
            std::cerr << "连接失败!" << std::endl;
            close(serverSocket);
            return;
        }

        std::cout << "客户端已连接!" << std::endl;

        // 使用 fork 创建子进程来处理客户端请求
        pid_t pid = fork();  // 创建子进程

        if (pid == -1) {  // 创建失败时返回 -1
            std::cerr << "创建子进程失败!" << std::endl;
            close(clientSocket);
            continue;
        }

        if (pid == 0) {  // 子进程部分
            close(serverSocket);  // 子进程不需要监听套接字
            handle_client(clientSocket);  // 处理客户端请求

            // 处理完后退出子进程（重要）
            exit(0);
        } else {  // 父进程部分
            close(clientSocket);  // 父进程不处理此连接，继续监听其他连接
        }
    }

    // 关闭监听套接字（通常不会执行到这里）
    close(serverSocket);
}

int main() {
    start_server();
    return 0;
}
