#include <iostream>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <algorithm>
#include <cstring>
#include <functional> // 包含 std::function

#define MAX_EVENTS 10

// 回调函数类型
typedef void (*CallbackFunction)(int fd);

class EpollServer {
public:
    EpollServer() : epoll_fd(-1) {}

    bool init() {
        epoll_fd = epoll_create1(0); // 创建 epoll 实例
        if (epoll_fd == -1) {
            std::cerr << "Failed to create epoll: " << strerror(errno) << std::endl;
            return false;
        }
        return true;
    }

    // 注册文件描述符和回调函数
    void addFd(int fd, CallbackFunction callback) {
        struct epoll_event ev;
        ev.events = EPOLLIN;  // 监听可读事件
        ev.data.fd = fd;
        fd_callbacks[fd] = callback;  // 保存回调函数

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
            std::cerr << "Failed to add fd to epoll: " << strerror(errno) << std::endl;
        }
    }

    // 事件循环
    void run() {
        struct epoll_event events[MAX_EVENTS];

        while (true) {
            int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
            if (nfds == -1) {
                std::cerr << "Epoll wait failed: " << strerror(errno) << std::endl;
                break;
            }

            // 遍历触发的事件
            for (int i = 0; i < nfds; ++i) {
                if (events[i].events & EPOLLIN) {
                    int fd = events[i].data.fd;
                    // 调用对应的回调函数处理事件
                    if (fd_callbacks.find(fd) != fd_callbacks.end()) {
                        fd_callbacks[fd](fd);  // 调用注册的回调函数
                    }
                }
            }
        }
    }

private:
    int epoll_fd;
    std::unordered_map<int, std::function<void(int)>> fd_callbacks; // 存储文件描述符与回调函数的映射
};

// 示例回调函数
void readCallback(int fd) {
    char buffer[512];
    int bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::cout << "Received: " << buffer << std::endl;
    } else if (bytesRead == 0) {
        std::cout << "Connection closed." << std::endl;
    } else {
        std::cerr << "Read error: " << strerror(errno) << std::endl;
    }
}

int main() {
    EpollServer server;
    if (!server.init()) {
        return 1;
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        std::cerr << "Pipe creation failed: " << strerror(errno) << std::endl;
        return 1;
    }

    // 向管道中写入数据
    const char* message = "Hello, epoll!";
    write(pipe_fd[1], message, strlen(message));

    // 注册管道读取事件
    server.addFd(pipe_fd[0], readCallback);

    // 运行 epoll 事件循环
    server.run();

    close(pipe_fd[0]);
    close(pipe_fd[1]);

    return 0;
}
