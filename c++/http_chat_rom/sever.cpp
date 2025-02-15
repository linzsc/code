#include <iostream>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "thread_poor.h"
#include <list>
#include <map>
#include "message.h"
#define MAX_EVENTS 10
#define PORT 12345
#define BUFF_SIZE 1024

// 回调函数类型
typedef void (*CallbackFunction)(int fd, uint32_t events);

// 在线用户列表

std::map<std::string, int>onlineMap;

ThreadPool& thread_pool = ThreadPool::instance(200);  // 使用线程池，线程数设为 4

// Epoll Reactor 服务器
class EpollReactor {
public:
    EpollReactor() : epoll_fd(-1) {}

    bool init() {
        epoll_fd = epoll_create1(0); // 创建 epoll 实例
        if (epoll_fd == -1) {
            std::cerr << "Failed to create epoll: " << strerror(errno) << std::endl;
            return false;
        }
        return true;
    }

    // 注册文件描述符和回调函数
    void addFd(int fd, uint32_t events, CallbackFunction callback) {
        struct epoll_event ev;
        ev.events = events;  // 监听的事件类型
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
                int fd = events[i].data.fd;
                uint32_t event = events[i].events;

                if (fd_callbacks.find(fd) != fd_callbacks.end()) {
                   fd_callbacks[fd](fd, event);  // 调用注册的回调函数
                }
            }
        }
    }

private:
    int epoll_fd;
    std::unordered_map<int, CallbackFunction> fd_callbacks; // 存储文件描述符与回调函数的映射
};

EpollReactor server;


void sendToClient(std::string & message) {
    Message msg;
    msg.deserialize(message);
    std::cout << "消息内容：" << msg.get_content() << std::endl;
    send(onlineMap[msg.get_recv()], msg.get_content().c_str(), msg.get_content().length(), 0);

}
void handleClientRead(int fd, uint32_t events) {
    // 将数据读取和写入操作都交给线程池处理
    thread_pool.commit([fd]() {
        char buffer[BUFSIZ];
        int bytesRead = read(fd, buffer, sizeof(buffer) - 1);
        std::string tmp=std::string(buffer);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            //std::cout << "Received from client: " << buffer << std::endl;

            // 处理数据

            //如果是上线，则添加到在线列表
            if (buffer[0] == '1') { 

                std::cout << "上线" << std::endl;
                // 添加到在线列表,除去首位的1
                std::string full(buffer);  // 将整个 buffer 转换为字符串
                std::string result = full.substr(1); 
                onlineMap[result] = fd;
                
               std::cout<<result<<"   "<<fd<<std::endl;
            }else{
                //sendToClient(tmp);
                Message msg = Message::deserialize(tmp);
                std::cout << "Received from client: " << tmp << std::endl;
                std::cout << "消息内容：" << msg.get_content() << std::endl;
                send(onlineMap[msg.get_recv()], tmp.c_str(), tmp.length(), 0);
            }
                
            
            // 处理完数据，并转发给其他客户端
            //sendToClient(buffer);

            //write(fd, "Message received", 17);
        } else if (bytesRead == 0) {
            std::cout << "Client disconnected." << std::endl;
            close(fd); // 客户端断开连接，关闭文件描述符
        } else {
            std::cerr << "Read error: " << strerror(errno) << std::endl;
        }
    });
}



// 处理客户端连接的回调函数
void handleNewConnection(int fd, uint32_t events) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);

    if (client_fd == -1) {
        std::cerr << "Accept failed: " << strerror(errno) << std::endl;
        return;
    }

    std::cout << "New connection from: " << inet_ntoa(client_addr.sin_addr) << std::endl;
    
    // 设置客户端连接为非阻塞
    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    // 注册客户端文件描述符的读事件
    // 当客户端发送数据时触发回调
    server.addFd(client_fd, EPOLLIN | EPOLLET, handleClientRead);
}




// 处理客户端读取数据的回调函数


// 处理标准输入的回调函数
void handleStdin(int fd, uint32_t events) {
    char buffer[512];
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::cout << "Received from stdin: " << buffer << std::endl;

        // 如果输入 "exit"，则关闭服务器
        if (strncmp(buffer, "exit", 4) == 0) {
            std::cout << "Exiting server..." << std::endl;
            exit(0);
        }
    }
}

int main() {
    if (!server.init()) {
        return 1;
    }
    // 创建监听套接字
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
        return 1;
    }

    // 设置 SO_REUSEADDR 选项，允许重用本地地址和端口
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "设置 SO_REUSEADDR 失败!" << std::endl;
        close(server_fd);
        return 0;
    }
    
    // 设置服务器地址
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 绑定和监听
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;
        return 1;
    }
    std::cout << "Server listening on port " << PORT << std::endl;
    // 设置服务器套接字为非阻塞
    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    // 注册服务器套接字的可读事件（用于接受新的连接）
    server.addFd(server_fd, EPOLLIN, handleNewConnection);

    // 注册标准输入的事件（用于控制服务器的退出）
    server.addFd(STDIN_FILENO, EPOLLIN, handleStdin);

    // 运行事件循环
    server.run();

    close(server_fd);
    return 0;
}
