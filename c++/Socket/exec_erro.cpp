#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <cstring>
#include <cstdlib>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>

#define SERVER_PORT 12345
#define MAX_EVENTS 10

// 获取并打印当前进程的文件描述符列表
void print_fds(const char* process_name) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/fd/", getpid());

    std::cout << process_name << " 的文件描述符列表：" << std::endl;

    DIR* dir = opendir(path);
    if (!dir) {
        std::cerr << "无法打开目录: " << path << std::endl;
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::cout << "文件描述符: " << entry->d_name << std::endl;
    }

    closedir(dir);
}

// 启动服务器
void start_server() {
    int server_fd, client_fd, epoll_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_size = sizeof(client_addr);

    // 创建套接字
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
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

    // **错误示范：没有使用 EPOLL_CLOEXEC**
    epoll_fd = epoll_create1(EPOLL_CLOEXEC);  // 没有使用 EPOLL_CLOEXEC
    if (epoll_fd == -1) {
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

    std::cout << "父进程的 epoll 文件描述符：" << epoll_fd << std::endl;
    print_fds("父进程（server.cpp）");

    // 使用 epoll 等待事件
    while (true) {
        struct epoll_event events[MAX_EVENTS];
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == server_fd) {
                client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_size);
                if (client_fd == -1) {
                    std::cerr << "接受连接失败!" << std::endl;
                    continue;
                }

                std::cout << "客户端连接成功!" << std::endl;

                pid_t pid = fork();
                if (pid == -1) {
                    std::cerr << "fork() 失败!" << std::endl;
                    close(client_fd);
                    continue;
                }

                if (pid == 0) {
                    // **子进程**
                    std::cout << "子进程执行 exec()!" << std::endl;
                    
                    // **传递 epoll_fd 给子进程**
                    char epoll_fd_str[10];
                    snprintf(epoll_fd_str, sizeof(epoll_fd_str), "%d", epoll_fd);
                    
                    execl("./bin/add", "add", epoll_fd_str, nullptr);

                    std::cerr << "exec 执行失败!" << std::endl;
                    exit(EXIT_FAILURE);
                } else {
                    // **父进程**
                    close(client_fd);
                    waitpid(pid, nullptr, 0);
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
