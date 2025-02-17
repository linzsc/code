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
std::string client_name="linz";

// 接收消息函数

/*
完整写解析http
*/

//解析http响应



void receive_data(int clientSocket) {
    char buffer[4096];
    ProtocolHeader header;
    std::vector<char> body;
    int state = 0; // 0: 读取协议头, 1: 读取消息体

    while (true) {
        if (state == 0) {
            // 读取协议头
            ssize_t bytesRead = recv(clientSocket, reinterpret_cast<char*>(&header), sizeof(header), 0);
            if (bytesRead <= 0) {
                std::cerr << "连接已关闭或接收失败!" << std::endl;
                break;
            }
            // 校验协议头
            if (header.magic != ProtocolHeader::MAGIC || header.version != 1) {
                std::cerr << "Invalid protocol header received!" << std::endl;
                break;
            }
            // 准备读取消息体
            body.resize(header.body_len);
            state = 1;
        } else {
            // 读取消息体
            ssize_t bytesRead = recv(clientSocket, body.data(), header.body_len, 0);
            if (bytesRead <= 0) {
                std::cerr << "连接已关闭或接收失败!" << std::endl;
                break;
            }
            // 分情况处理处理消息体
            if(header.msg_type == ProtocolHeader::MsgType::HTTP_RESPONSE){

            }
            else if (header.msg_type == ProtocolHeader::MsgType::CHAT_RESPONSE)
            {
                
                std::string body_str(body.begin(), body.end());
                size_t body_len = body_str.find("\r\n\r\n");
                Message msg = Message::deserialize(std::string(body.begin()+body_len+4, body.end()));
                if(msg.type == MessageType::GroupChat){
                    std::cout << "[gropupchat Sender:<" << msg.get_sender() << ">]: " << msg.get_content() << std::endl;
                }
                else if(msg.type == MessageType::PrivateChat){
                    std::cout << "[Sender:<" << msg.get_sender() << ">]: " << msg.get_content() << std::endl;
                }
                
                std::cout << "Message Timestamp:" << msg.get_timestamp() << std::endl;
               
            }
            
            state = 0;
        }
    }
}


void sendHttpRequest(int cfd, const std::string& method, const std::string& path, const std::string& body) {
    std::ostringstream oss;
    oss << method << " " << path << " HTTP/1.1\r\n";
    oss << "Content-Type: application/json\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "\r\n";
    oss << body; 

    std::string request = oss.str();//这是body部分
    std::cout<<request<<std::endl;
    ProtocolHeader header;
    header.setMessageType(ProtocolHeader::MsgType::HTTP_REQUEST);
    header.setBodyLength(request.size());
    //std::cout<<"head_size: "<<header.serialize().size()<<std::endl;
    std::string complete_message = header.serialize() + request;
    //std::string complete_message = request;
    std::cout<<complete_message.size()<<std::endl;

    send(cfd, complete_message.c_str(), complete_message.size(), 0);
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
        std::string body = msg.serialize();
        ProtocolHeader header;
        header.setMessageType(ProtocolHeader::MsgType::CHAT_MESSAGE);
        header.setBodyLength(body.size());
        header.print(); // 打印协议头信息

        // 组合协议头和消息体
        std::string complete_message = header.serialize() + body;
        std::cout<<"message you sent:  "+complete_message<<std::endl;
        // 发送完整消息
        send(clientSocket, complete_message.c_str(), complete_message.size(), 0);
        std::cout << "发送成功\n";
    }
}



void user_regist(int cfd) {
    std::string username = "linz";
    std::string password = "0128";
    std::string json_body = "{\"username\":\"" + username + "\", \"password\":\"" + password + "\"}";

    // 构造 HTTP 注册请求
    sendHttpRequest(cfd, "POST", "/register", json_body);
}
void user_login(int cfd) {
    std::string username = "linz";
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
    serverAddr.sin_port = htons(12345);

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
        std::cerr << "连接失败!" << std::endl;
        close(clientSocket);
        return;
    }

    std::cout << "成功连接到服务器!" << std::endl;

    std::thread recvThread(receive_data, clientSocket);
    std::thread sendThread(send_data, clientSocket);

    user_regist(clientSocket);
    user_login(clientSocket);
    //send_name(clientSocket);
    
    

    sendThread.join();
    recvThread.detach();
    
    close(clientSocket);
}

int main() {
    start_client();
    return 0;
}