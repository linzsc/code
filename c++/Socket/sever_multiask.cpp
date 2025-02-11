#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <cstring>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// 设置文件描述符为非阻塞模式
void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    // 创建套接字
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed!" << std::endl;
        return -1;
    }

    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定套接字
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Bind failed!" << std::endl;
        return -1;
    }

    // 监听连接
    if (listen(server_fd, 5) < 0) {
        std::cerr << "Listen failed!" << std::endl;
        return -1;
    }

    // 设置非阻塞模式
    setNonBlocking(server_fd);

    // 创建文件描述符集合
    fd_set read_fds;
    FD_ZERO(&read_fds);

    int client_fd = -1;

    while (true) {
        // 清空并加入监听套接字
        FD_SET(server_fd, &read_fds);
        
        // 使用 select 来进行非阻塞 I/O
        int activity = select(server_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0) {
            std::cerr << "Select failed!" << std::endl;
            continue;
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            // 服务器套接字有新连接请求
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                std::cerr << "Accept failed!" << std::endl;
            } else {
                std::cout << "New client connected!" << std::endl;
                setNonBlocking(client_fd); // 将客户端套接字设置为非阻塞
            }
        }

        // 如果已经有客户端连接，检查是否有数据可读
        if (client_fd != -1 && FD_ISSET(client_fd, &read_fds)) {
            char buffer[BUFFER_SIZE];
            int bytes_read = read(client_fd, buffer, sizeof(buffer));

            if (bytes_read > 0) {
                // 客户端发送了数据
                buffer[bytes_read] = '\0'; // 确保字符串以 \0 结束
                std::cout << "Received from client: " << buffer << std::endl;
            } else if (bytes_read == 0) {
                // 客户端关闭连接
                std::cout << "Client disconnected!" << std::endl;
                close(client_fd);
                client_fd = -1;
            } else {
                // 如果没有数据，继续轮询
                std::cerr << "Error reading data from client!" << std::endl;
            }
        }

        // 非阻塞忙轮询：每次轮询后稍作休息，减少 CPU 占用
        usleep(100000);  // 休眠 100ms
    }

    close(server_fd);
    return 0;
}
