#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sys/select.h>
#include <string.h>
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024   


/*
select 函数的参数：
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
nfds：需要监视的文件描述符集合中，最大文件描述符加 1。
readfds：需要监视的可读文件描述符集合。
writefds：需要监视的可写文件描述符集合。
exceptfds：需要监视的异常文件描述符集合。
timeout：等待的超时时间，如果为 NULL，则表示无限等待。

返回值：
成功时，返回就绪的文件描述符数量；如果超时，返回 0；如果出错，返回 -1。

select相关函数：
FD_ZERO(fd_set *set)：将 set 集合清空。
FD_SET(int fd, fd_set *set)：将文件描述符 fd 添加到 set 集合中。
FD_CLR(int fd, fd_set *set)：将文件描述符 fd 从 set 集合中移除。
FD_ISSET(int fd, fd_set *set)：判断文件描述符 fd 是否在 set 集合中。

select有两个缺点：
1. 每次调用 select 都需要将文件描述符集合从用户空间拷贝到内核空间，当文件描述符数量较多时，会严重影响性能。
2. select 只能监视有限数量的文件描述符，一般为 1024。

*/
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

    // 使用 select 监听多个文件描述符
    fd_set master_set, read_fds;
    FD_ZERO(&master_set);
    FD_SET(serverSocket, &master_set);  // 添加监听套接字到 master set

    int max_fd = serverSocket;

    while (true) {
        printf("while");
        read_fds = master_set;  // 每次调用 select 前复制 master_set    
        //每次监听完select后，select只会保留read_fds中有数据传输的文件描述符，其余的文件描述符会被清空

        // 使用 select 来监视文件描述符
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            std::cerr << "Select 错误!" << std::endl;
            continue;
        }

        // 检查是否有新连接
        if (FD_ISSET(serverSocket, &read_fds)) {
            clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientSize);
            printf("clientSocket= %d\n",clientSocket);
            if (clientSocket == -1) {
                std::cerr << "连接失败!" << std::endl;
                continue;
            }

            // 设置客户端套接字为非阻塞
            FD_SET(clientSocket, &master_set);  // 将新客户端套接字加入到 master set

            if (clientSocket > max_fd) {
                max_fd = clientSocket;  // 更新 max_fd
            }

            // 打印客户端的 IP 地址和端口
            char clientIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
            std::cout << "客户端 IP: " << clientIp << ", 端口: " << ntohs(clientAddr.sin_port) << " 已连接!" << std::endl;
        }

        // 遍历所有客户端套接字，检查是否有数据可读
        for (int i = 0; i <= max_fd; ++i) {
            if (FD_ISSET(i, &read_fds) && i != serverSocket) {  // 跳过 serverSocket
                char buffer[BUFFER_SIZE];
                int recvSize = recv(i, buffer, sizeof(buffer), 0);
                if (recvSize > 0) {
                    buffer[recvSize] = '\0';  // 添加字符串结束符
                    std::cout << "收到来自客户端的数据: " << buffer << std::endl;
                    const char* massage= "服务器收到你的消息了   !";
                    // 向客户端发送回应
                    send(i, massage, strlen(massage), 0);
                } else if (recvSize == 0) {
                    std::cout << "客户端关闭连接!" << std::endl;
                    close(i);
                    FD_CLR(i, &master_set);  // 从 master set 中移除该客户端
                } else {
                    std::cerr << "接收数据失败!" << std::endl;
                    close(i);
                    FD_CLR(i, &master_set);  // 从 master set 中移除该客户端
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
