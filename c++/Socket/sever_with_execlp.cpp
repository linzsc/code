#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <cstring>
#include <cstdlib>
#include <sys/wait.h>

#define SERVER_PORT 12345
#define MAX_EVENTS 10

// 处理客户端连接的函数
void handle_client(int client_fd) {
    char buffer[1024];
    int n;

    while ((n = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
        buffer[n] = '\0';
        std::cout << "收到客户端消息: " << buffer << std::endl;
        const char *massage="服务器已收到消息";
        send(client_fd, massage, strlen(massage), 0);
    }

    if (n == 0) {
        std::cout << "客户端关闭连接!" << std::endl;
    } else {
        std::cerr << "接收数据失败!" << std::endl;
    }

    close(client_fd);
}

// 启动服务器并处理客户端连接
void start_server() {
    int server_fd, client_fd, epoll_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_size = sizeof(client_addr);

    // 创建套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        std::cerr << "套接字创建失败!" << std::endl;
        return;
    }

    // 配置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // 绑定套接字
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "绑定失败!" << std::endl;
        close(server_fd);
        return;
    }

    // 启动监听
    if (listen(server_fd, 5) == -1) {
        std::cerr << "监听失败!" << std::endl;
        close(server_fd);
        return;
    }

    std::cout << "服务器启动，监听端口 " << SERVER_PORT << " ..." << std::endl;

    // 创建 epoll 实例，使用 EPOLL_CLOEXEC 标志
    if ((epoll_fd = epoll_create1(0)) == -1) {
        std::cerr << "创建 epoll 实例失败!" << std::endl;
        close(server_fd);
        return;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_fd;

    // 将服务器套接字加入到 epoll 实例
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        std::cerr << "将服务器套接字加入 epoll 失败!" << std::endl;
        close(server_fd);
        return;
    }

    // 使用 epoll 等待事件
    while (true) {
        struct epoll_event events[MAX_EVENTS];
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == server_fd) {
                // 有新的客户端连接
                client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_size);
                if (client_fd == -1) {
                    std::cerr << "接受连接失败!" << std::endl;
                    continue;
                }

                std::cout << "客户端连接成功!" << std::endl;

                // 在此模拟通过 fork() 创建新进程并执行 exec()
                pid_t pid = fork();
                if (pid == -1) {
                    std::cerr << "fork() 失败!" << std::endl;
                    close(client_fd);
                    continue;
                }

                if (pid == 0) {
                    // 子进程
                    std::cout << "子进程执行 exec()!" << std::endl;
                    
                    // 在子进程中执行 exec，调用同目录下的 add 可执行文件
                    if (execl("./bin/add", "add", nullptr) == -1) {
                        std::cerr << "exec 执行失败!" << std::endl;
                        close(client_fd);
                        exit(EXIT_FAILURE);
                    }
                } else {
                    // 父进程
                    // 在父进程中继续处理客户端请求
                    handle_client(client_fd);
                    close(client_fd);
                    waitpid(pid, nullptr, 0);  // 等待子进程退出
                }
            }
        }
    }

    close(server_fd);
    close(epoll_fd);
}

int main() {
    start_server();
    return 0;
}
