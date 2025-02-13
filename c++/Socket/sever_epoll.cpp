#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/epoll.h>
#define MAX_CLIENTS 10
#define BUFFER_SIZE 2
/*
epoll 实现多客户端通信
epoll的API接口函数主要有以下几个：
epoll_create：创建一个epoll实例，返回一个文件描述符。（旧版，不推荐）
epoll_ctl：向epoll实例中添加、修改或删除文件描述符。
epoll_wait：等待epoll实例中的文件描述符变为就绪状态，返回就绪的文件描述符数量。
epoll_create1：与epoll_create类似，但提供了更多的选项。（新版，推荐使用）


各个接口的详细说明如下：

epoll_create：创建一个epoll实例，返回一个文件描述符。该函数的原型如下：
int epoll_create(int size);
参数size表示需要监听的文件描述符的数量，但该参数在Linux 2.6.8之后已经不再使用，可以设置为任意值。

epoll_ctl：向epoll实例中添加、修改或删除文件描述符。该函数的原型如下：
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
参数epfd是epoll实例的文件描述符，op表示操作类型，fd是要操作的文件描述符，event表示要设置的事件类型。op的取值可以是以下几种：
EPOLL_CTL_ADD：向epoll实例中添加文件描述符。
EPOLL_CTL_MOD：修改epoll实例中文件描述符的事件类型。
EPOLL_CTL_DEL：从epoll实例中删除文件描述符。


epoll_event 结构体用于描述要监听的事件类型，其定义如下：
struct epoll_event {
    uint32_t events;  // 事件类型
    epoll_data_t data;  // 用户数据
}

epoll_data_t 结构体用于存储用户数据，其定义如下：
typedef union epoll_data {
    void* ptr;  // 指向用户数据的指针
    
}


epoll_wait：等待epoll实例中的文件描述符变为就绪状态，返回就绪的文件描述符数
epoll工作的两种方式：
水平触发（LT）：默认模式，当文件描述符变为就绪状态时，epoll_wait会返回该文件描述符，即使没有读取完数据。应用程序需要继续读取数据，直到返回0为止。
边缘触发（ET）：当文件描述符变为就绪状态时，epoll_wait只会返回一次，即使没有读取完数据。应用程序需要一次性读取完所有数据，否则可能会错过后续的数据。
*/

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
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "设置 SO_REUSEADDR 失败!" << std::endl;
        close(serverSocket);
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
    
    // 创建 epoll 实例
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "Epoll 创建失败!" << std::endl;
        close(serverSocket);
        return;
    }

    // 添加服务器套接字到 epoll 实例
    struct epoll_event ev, events[MAX_CLIENTS + 1];
    ev.events = EPOLLIN;  // 监听可读事件
    ev.data.fd = serverSocket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket, &ev) == -1) {
        std::cerr << "Epoll 添加服务器套接字失败!" << std::endl;
        close(serverSocket);
        close(epoll_fd);
        return;
    }

    while (true) {
        // 使用 epoll_wait 等待事件发生
        int activity = epoll_wait(epoll_fd, events, MAX_CLIENTS + 1, -1);
        if (activity < 0) {
            std::cerr << "Epoll 等待事件错误!" << std::endl;
            continue;
        }

        // 遍历所有事件
        for (int i = 0; i < activity; ++i) {
            if (events[i].data.fd == serverSocket) {
                // 处理新连接
                clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientSize);
                if (clientSocket == -1) {
                    std::cerr << "连接失败!" << std::endl;
                    continue;
                }

                // 设置客户端套接字为非阻塞
                int flags = fcntl(clientSocket, F_GETFL, 0);
                fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
                ev.events = EPOLLIN | EPOLLET;  // 使用边缘触发
                ev.data.fd = clientSocket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clientSocket, &ev) == -1) {
                    std::cerr << "Epoll 添加客户端套接字失败!" << std::endl;
                    close(clientSocket);
                    continue;
                }

                // 打印客户端的 IP 地址和端口
                char clientIp[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
                std::cout << "客户端 IP: " << clientIp << ", 端口: " << ntohs(clientAddr.sin_port) << " 已连接!" << std::endl;
            } else {
                std::string message_all; 
                while (1)
                {
                    // 处理客户端发送的数据
                    char buffer[BUFFER_SIZE];
                    int recvSize = recv(events[i].data.fd, buffer, sizeof(buffer), 0);
                    if (recvSize > 0)
                    {
                        buffer[recvSize] = '\0'; // 添加字符串结束符
                        std::cout << "收到来自客户端的数据: " << buffer << std::endl;
                        //const char *message = "服务器收到你的消息了!";
                        //char *massage_all = new char[strlen(buffer)  + 1];
                        // 向客户端发送回应
                        //send(events[i].data.fd, message, strlen(message), 0);
                    
                        message_all+=buffer;
                    }
                    else if (recvSize == 0)
                    {
                        std::cout << "客户端关闭连接!" << std::endl;
                        // close(events[i].data.fd);
                        //  从 epoll 实例中删除客户端套接字
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) == -1)
                        { // 先删除，再关闭，防止出现错误
                            std::cerr << "Epoll 删除客户端套接字失败!" << std::endl;
                        }
                        close(events[i].data.fd);
                    }
                    else
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // 没有数据可读，可以退出
                            std::cout<<"没有数据可读，可以退出"<<std::endl;
                            std::cout<<"数据为"<<message_all<<std::endl;
                            send(events[i].data.fd, message_all.c_str(), message_all.length(), 0);
                            break;
                        }
                        std::cerr << "接收数据失败!" << std::endl;
                        // 关闭客户端套接字并从 epoll 实例中删除
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) == -1) {
                            std::cerr << "Epoll 删除客户端套接字失败!" << std::endl;
                        }
                        close(events[i].data.fd);
                        break; // 退出 while 循环
                    }
                }
            }
        }
    }

    // 关闭服务器套接字和 epoll 实例
    close(serverSocket);
    close(epoll_fd);
}

int main() {
    start_server();
    return 0;
}
