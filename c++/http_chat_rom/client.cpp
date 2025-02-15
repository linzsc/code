#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <mutex>
#include "message.h"

void receive_data(int clientSocket) {
    char buffer[1024];
    while (true) {
        int recvSize = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (recvSize <= 0) {
            std::cerr << "连接已关闭或接收失败!" << std::endl;
            break;
        }
        //buffer[recvSize] = '\0';  // 添加 null 结尾
        Message msg = Message::deserialize(buffer);
        std::cout <<msg.get_sender()+": "<< msg.get_content() << std::endl;
        std::cout << msg.timestamp << std::endl;
    }
}

void send_data(int clientSocket) {
    std::string message;
    while (true) {
        std::getline(std::cin, message);
        if (message == "exit") {
            break;
        }
        Message msg(MessageType::PrivateChat, message, "aaaa", "bbbb");
        send(clientSocket, msg.serialize().c_str(), msg.serialize().length(), 0);
        std::cout << "发送成功\n";
    }
}

void start_client() {
    int clientSocket;
    struct sockaddr_in serverAddr;

    // 创建套接字
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Socket 创建失败!" << std::endl;
        return;
    }

    // 配置服务器地址
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // 连接到本地地址
    serverAddr.sin_port = htons(12345);  // 使用端口 12345

    // 连接到服务器
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "连接失败!" << std::endl;
        close(clientSocket);
        return;
    }
    std::cout << "成功连接到服务器!" << std::endl;
    send(clientSocket, "1aaaa", strlen("1aaaa"), 0);
    printf("Iam aaaa\n");
    
    // 创建接收和发送线程
    std::thread recvThread(receive_data, clientSocket);
    std::thread sendThread(send_data, clientSocket);

    // 等待线程结束
    sendThread.join();
    recvThread.detach();  // 接收线程可以独立运行

    // 关闭连接
    close(clientSocket);
}

int main() {
    start_client();
    return 0;
}