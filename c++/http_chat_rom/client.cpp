#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include "http_header.h"
#include "message.h"

// 客户端名称
std::string client_name = "linz";

// 解析HTTP响应

// 接收消息函数
void receive_data(int clientSocket) {
    char buffer[4096];
    while (true) {
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        std::cout << "接收到的内容:\n" << buffer << std::endl;
        if (bytesRead <= 0) {
            if (bytesRead == 0) {
                std::cout << "服务器断开连接!" << std::endl;
            } else {
                std::cerr << "接收数据失败!" << std::endl;
            }
            break;
        }

        buffer[bytesRead] = '\0';
        std::string response(buffer);

        // 解析HTTP响应
        HttpResponse res = HttpParser::parseHttpResponse(response);
        HttpParser::printHttpResponse(res);
        /*
        // 处理WebSocket消息
        if (status_code == "101 Switching Protocols") {
            std::cout << "WebSocket连接已建立!" << std::endl;
        } else if (status_code == "200 OK") {
            std::cout << "HTTP请求成功!" << std::endl;
        } else {
            std::cout << "HTTP请求失败: " << status_code << std::endl;
        }
            */
    }
}

void sendHttpRequest(int cfd, const std::string& method, const std::string& path, const std::string& body) {
    std::ostringstream oss;
    oss << method << " " << path << " HTTP/1.1\r\n";
    oss << "Content-Type: application/json\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "\r\n";
    oss << body;

    std::string request = oss.str();
    std::cout << "发送HTTP请求: " << request << std::endl;
    send(cfd, request.c_str(), request.size(), 0);
}

// 发送消息函数
void send_data(int clientSocket) {
    std::string message;
    while (true) {
        std::getline(std::cin, message);
        if (message == "exit") {
            break;
        }
        Message msg(MessageType::GroupChat, client_name, "linz", message);
        std::string complete_message = msg.serialize();
        std::cout << "发送消息: " << complete_message << std::endl;
        send(clientSocket, complete_message.c_str(), complete_message.size(), 0);
    }
}

void user_regist(int cfd) {
    std::string username = "linz1";
    std::string password = "0128";
    std::string json_body = "{\"username\":\"" + username + "\", \"password\":\"" + password + "\"}";

    // 构造 HTTP 注册请求
    sendHttpRequest(cfd, "POST", "/register", json_body);
}

void user_login(int cfd) {
    std::string username = "linz1";
    std::string password = "0128";
    std::string json_body = "{\"username\":\"" + username + "\", \"password\":\"" + password + "\"}";

    // 构造 HTTP 登录请求
    sendHttpRequest(cfd, "POST", "/login", json_body);
}

// 客户端启动函数
void start_client() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Socket 创建失败!" << std::endl;
        return;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(8080);

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
        std::cerr << "连接失败!" << std::endl;
        close(clientSocket);
        return;
    }

    std::cout << "成功连接到服务器!" << std::endl;

    std::thread recvThread(receive_data, clientSocket);
    std::thread sendThread(send_data, clientSocket);

    user_regist(clientSocket);
    sleep(1);
    user_login(clientSocket);

    sendThread.join();
    recvThread.detach();

    close(clientSocket);
}

int main() {
    start_client();
    return 0;
}