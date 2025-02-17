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
#include <map>
#include "http_header.h"
#include "message.h"
#include <regex>
#include "simple_json.h"
#define MAX_EVENTS 10
#define PORT 12345
#define BUFF_SIZE 1024

// 回调函数类型
typedef void (*CallbackFunction)(int fd, uint32_t events);

// 在线用户列表

std::map<std::string, int>online_name;
std::map<int,std::string>online_fd;

ThreadPool& thread_pool = ThreadPool::instance(200);  // 使用线程池

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


void sendToClient(int fd,std::string & message) {//后续还需要区别群聊和私聊，目前只是私聊
    //验证在线列表中是否由该用户
    Message msg;
    msg.deserialize(message);
    std::cout << "消息内容：" << msg.get_content() << std::endl;
    if(online_fd[fd]!=msg.get_recv()||online_fd.find(fd)==online_fd.end()){
        std::cout<<"该用户为非法用户！！不在在线列表中"<<std::endl;
        return;
    }
    
    send(online_name[msg.get_recv()], msg.get_content().c_str(), msg.get_content().length(), 0);

}



enum class ReadState {
    HEADER,  // 正在读取协议头
    BODY     // 正在读取消息体
};

// 客户端连接上下文
struct ClientContext {
    ReadState state = ReadState::HEADER;
    char buffer[4096];  // 临时缓冲区，用于累积接收的数据
    size_t buffer_pos = 0;  // 缓冲区当前存储位置
    ProtocolHeader header;  // 协议头
    char* body_buf = nullptr;  // 消息体缓冲区
    size_t body_len = 0;      // 当前消息体长度
    size_t body_capacity = 0; // 消息体缓冲区容量

    ~ClientContext() {
        // 释放消息体缓冲区
        if (body_buf) {
            free(body_buf);
        }
    }
};


// 全局客户端上下文映射
std::unordered_map<std::string, std::string> user_db;
std::unordered_map<int, ClientContext> client_contexts;

void sendHttpResponse(int fd, const std::string& status_code, const std::string& content_type, const std::string& body) {
    // 构造 HTTP 响应头
    ProtocolHeader  header;
    header.version=1;
    header.setMessageType(ProtocolHeader::MsgType::HTTP_RESPONSE);
    
    std::string response=response += "HTTP/1.1 " + status_code ; // 状态行
    if(status_code=="200"){
        response+="ok \r\n";
    }
    else{
        response+="fail \r\n";
    }
    response += +"Content-Type: " + content_type + "\r\n";        // 内容类型
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n"; // 内容长度
    response += "Connection: close\r\n";                          // 关闭连接
    response += "\r\n";                                           // 空行，表示响应头结束

    // 构造 HTTP 响应体
    response += body;
    header.setBodyLength(response.size());
    response=header.serialize()+response;
    // 将响应发送到客户端
    if (send(fd, response.c_str(), response.size(), 0) < 0) {
        std::cerr << "Failed to send HTTP response." << std::endl;
    }
}


void broadcastMessage(int fd,std::string content){

}


void user_register(int fd, std::string ctx_body) {
    //std::cout << "??????????????????"  << std::endl;

    SimpleJSONParser parser(ctx_body);
    std::map<std::string, std::string> userData = parser.parse();

    std::string username = userData["username"];
    std::string password = userData["password"];
    // 模拟注册进入数据库
    if(user_db.find(username)==user_db.end()){
        user_db[username] = password;
    }
    else{
        sendHttpResponse(fd, "400", "application/json", "{\"status\":\"这个id已经注册过了！\"}");
    }
    //user_db[username] = password;

    std::cout << "Registered user: " << username << ", password: " << password << std::endl;

    sendHttpResponse(fd, "200", "application/json", "{\"status\":\"ok\"}");
}

void user_login(int fd, std::string ctx_body)
{
    SimpleJSONParser parser(ctx_body);
    std::map<std::string, std::string> userData = parser.parse();

    std::string username = userData["username"];
    std::string password = userData["password"];

    // 模拟登录验证
    if (user_db.find(username) != user_db.end() && user_db[username] == password)
    {
        //用户在数据库内，且登录加入到在线列表中，
        online_name[username] = fd;
        online_fd[fd] = username;
        std::cout<<username<<"成功登录\n";
        sendHttpResponse(fd, "200", "text/html", "Login successful");
    }
    else
    {
        sendHttpResponse(fd, "401", "text/html", "Unauthorized");
    }
}


/*
下一步：
实现消息转发
优化onMessage函数，使ctx_body只有内容而没有前面的信息
实现服务器的的sendHttpResponse函数


*/

void onMessage(int fd, ProtocolHeader ctx_header, std::string ctx_body) {
    ctx_header.print();
    std::cout << "body: " << ctx_body << std::endl;
    
    try {
        printf("status:%d\n",ctx_header.msg_type);
        switch (ctx_header.msg_type) {
            case ProtocolHeader::MsgType::HTTP_REQUEST: {
                // 解析 HTTP 请求
                std::regex path_regex(R"((^\w+) /([^ ]+) HTTP/\d+\.\d+)");
                std::smatch matches;
                if (std::regex_search(ctx_body, matches, path_regex)) {
                    std::string request_method = matches[1];
                    std::string request_path = matches[2];
                    std::cout<<"\n\n\n\n"+request_path+'\n'<<std::endl;
                    std::transform(request_path.begin(), request_path.end(), request_path.begin(), ::tolower);


                    // 提取 JSON 数据
                    std::string json_data;
                    size_t body_start = ctx_body.find("\r\n\r\n");
                    if (body_start != std::string::npos) {
                        body_start += 4; // 跳过 "\r\n\r\n"
                        json_data = ctx_body.substr(body_start, ctx_body.size()-body_start);
                        //直接用ctx_body的size-
                    } else {
                        sendHttpResponse(fd, "400", "text/html", "Bad Request: Missing body");
                        return;
                    }

                    
                    // 处理 HTTP 请求路径
                    if (request_path == "login") {
                        user_login(fd, json_data);
                    } else if (request_path == "register") {
                        user_register(fd, json_data);
                    } else if (request_path.rfind("static/", 0) == 0) {
                        std::string resource = request_path.substr(8);
                        sendHttpResponse(fd, "200", "text/html", "<html><body><h1>Static Resource</h1></body></html>");
                    } else {
                        sendHttpResponse(fd, "404", "text/html", "Not Found");
                    }
                } else {
                    sendHttpResponse(fd, "400", "text/html", "Bad Request");
                }
                break;
            }
            case ProtocolHeader::MsgType::CHAT_MESSAGE: {
                sendToClient(fd, ctx_body);
                break;
            }
            case ProtocolHeader::MsgType::LOGIN: {
                send(fd, "{\"status\":\"ok\"}", 14, 0);
                break;
            }
            case ProtocolHeader::MsgType::REGISTER: {
                send(fd, "{\"status\":\"registered\"}", 21, 0);
                break;
            }
            case ProtocolHeader::MsgType::LOGOUT: {
                std::string username = online_fd[fd];
                if (!username.empty()) {
                    online_name.erase(username);
                    online_fd.erase(fd);
                }
                send(fd, "{\"status\":\"logged out\"}", 20, 0);
                close(fd);
                break;
            }
            default:
                std::cerr << "Unknown message type: " << static_cast<int>(ctx_header.msg_type) << std::endl;
                break;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in onMessage: " << e.what() << std::endl;
        try {
            close(fd);
        } catch (...) {
            std::cerr << "Failed to close file descriptor." << std::endl;
        }
    }
}
void handleClientRead(int fd, uint32_t events) {
    thread_pool.commit([fd]() {
        if (client_contexts.find(fd) == client_contexts.end()) {
            ClientContext ctx;
            client_contexts[fd] = ctx;
        }

        ClientContext& ctx = client_contexts[fd];

        while (true) {
            char chunk[4096];
            int bytesRead = read(fd, chunk, sizeof(chunk));
            
            if (bytesRead <= 0) {
                if (bytesRead == 0) {
                    std::cout << "Client disconnected." << std::endl;
                    client_contexts.erase(fd);
                } else if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    std::cerr << "Read error: " << strerror(errno) << std::endl;
                }
                break;
            }

            // 将接收到的数据累积到临时缓冲区
            memcpy(ctx.buffer + ctx.buffer_pos, chunk, bytesRead);
            ctx.buffer_pos += bytesRead;

            // 处理累积的数据
            while (ctx.buffer_pos > 0) {
                try {
                    if (ctx.state == ReadState::HEADER) {
                        // 尝试解析 ProtocolHeader
                        size_t header_size = sizeof(ProtocolHeader);
                        if (ctx.buffer_pos >= header_size) {
                            ProtocolHeader header;
                            memcpy(&header, ctx.buffer, header_size);
                            ctx.header = header;

                            // 校验魔数和版本
                            if (header.magic != ProtocolHeader::MAGIC || header.version != 1) {
                                throw std::runtime_error("Invalid protocol header");
                            }

                            // 切换到读取消息体状态
                            ctx.state = ReadState::BODY;

                            // 清空缓冲区，保留未处理的字节
                            memcpy(ctx.buffer, ctx.buffer + header_size, ctx.buffer_pos - header_size);
                            ctx.buffer_pos -= header_size;

                            // 如果消息体长度为0，直接处理空消息体
                            if (header.body_len == 0) {
                                ctx.state = ReadState::HEADER;
                                onMessage(fd, ctx.header, "");
                            }
                        } else {
                            // 数据不足，继续等待
                            break;
                        }
                    } else if (ctx.state == ReadState::BODY) {
                        // 尝试读取消息体
                        size_t body_bytes_needed = ctx.header.body_len;
                        size_t available_bytes = ctx.buffer_pos;

                        if (available_bytes >= body_bytes_needed) {
                            // 确保有足够空间存储消息体
                            if (ctx.body_capacity < body_bytes_needed) {
                                // 重新分配内存
                                size_t new_capacity = std::max(body_bytes_needed, ctx.body_capacity * 2);
                                char* new_buf = static_cast<char*>(realloc(ctx.body_buf, new_capacity));
                                if (!new_buf) {
                                    throw std::runtime_error("Memory allocation failed");
                                }
                                ctx.body_buf = new_buf;
                                ctx.body_capacity = new_capacity;
                            }

                            // 从缓冲区中提取消息体
                            memcpy(ctx.body_buf + ctx.body_len, ctx.buffer, body_bytes_needed);
                            ctx.body_len += body_bytes_needed;

                            // 处理消息
                            onMessage(fd, ctx.header, std::string(ctx.body_buf, ctx.body_len));

                            // 清空缓冲区，保留未处理的字节
                            memcpy(ctx.buffer, ctx.buffer + body_bytes_needed, ctx.buffer_pos - body_bytes_needed);
                            ctx.buffer_pos -= body_bytes_needed;

                            // 切换到读取协议头状态
                            ctx.state = ReadState::HEADER;

                            // 释放消息体缓冲区
                            free(ctx.body_buf);
                            ctx.body_buf = nullptr;
                            ctx.body_len = 0;
                            ctx.body_capacity = 0;
                        } else {
                            // 累积消息体
                            if (ctx.body_capacity < ctx.body_len + available_bytes) {
                                // 重新分配内存
                                size_t new_capacity = ctx.body_capacity + available_bytes;
                                char* new_buf = static_cast<char*>(realloc(ctx.body_buf, new_capacity));
                                if (!new_buf) {
                                    throw std::runtime_error("Memory allocation failed");
                                }
                                ctx.body_buf = new_buf;
                                ctx.body_capacity = new_capacity;
                            }

                            memcpy(ctx.body_buf + ctx.body_len, ctx.buffer, available_bytes);
                            ctx.body_len += available_bytes;

                            // 清空缓冲区
                            ctx.buffer_pos = 0;

                            // 需要更多数据才能完成消息体
                            break;
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error processing client data: " << e.what() << std::endl;
                    close(fd);
                    client_contexts.erase(fd);
                    break;
                }
            }
        }
    });
}
/*
void handleClientRead(int fd, uint32_t events) {
    thread_pool.commit([fd]() {
        if (client_contexts.find(fd) == client_contexts.end()) {
            ClientContext ctx;
            client_contexts[fd] = ctx;
        }

        ClientContext& ctx = client_contexts[fd];

        while (true) {
            char chunk[4096];
            int bytesRead = read(fd, chunk, sizeof(chunk));
            
            if (bytesRead <= 0) {
                if (bytesRead == 0) {
                    std::cout << "Client disconnected." << std::endl;
                    client_contexts.erase(fd);
                } else if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    std::cerr << "Read error: " << strerror(errno) << std::endl;
                }
                break;
            }

            // 将接收到的数据累积到临时缓冲区
            memcpy(ctx.buffer + ctx.buffer_pos, chunk, bytesRead);
            ctx.buffer_pos += bytesRead;

            // 处理累积的数据
            while (ctx.buffer_pos > 0) {
                try {
                    if (ctx.state == ReadState::HEADER) {
                        // 尝试解析 ProtocolHeader
                        size_t header_size = sizeof(ProtocolHeader);
                        if (ctx.buffer_pos >= header_size) {
                            ProtocolHeader header;
                            memcpy(&header, ctx.buffer, header_size);
                            ctx.header = header;

                            // 校验魔数和版本
                            if (header.magic != ProtocolHeader::MAGIC || header.version != 1) {
                                throw std::runtime_error("Invalid protocol header");
                            }

                            // 切换到读取消息体状态
                            ctx.state = ReadState::BODY;
                            ctx.body.resize(header.body_len);  // 预分配消息体空间

                            // 清空缓冲区，保留未处理的字节
                            memcpy(ctx.buffer, ctx.buffer + header_size, ctx.buffer_pos - header_size);
                            ctx.buffer_pos -= header_size;

                            // 如果消息体长度为0，直接处理空消息体
                            if (header.body_len == 0) {
                                ctx.state = ReadState::HEADER;
                                onMessage(fd, ctx.header, "");
                            }
                        } else {
                            // 数据不足，继续等待
                            break;
                        }
                    } else if (ctx.state == ReadState::BODY) {
                        // 尝试读取消息体
                        size_t body_bytes_needed = ctx.header.body_len - ctx.body.size();
                        size_t available_bytes = ctx.buffer_pos;

                        if (available_bytes >= body_bytes_needed) {
                            // 从缓冲区中提取消息体
                            memcpy(ctx.body.data() + ctx.body.size(), ctx.buffer, body_bytes_needed);
                            ctx.body.resize(ctx.header.body_len);
                            //std::cout << "body:  " << std::endl;
                            //std::cout << std::string(ctx.body.begin(), ctx.body.end()) << std::endl;

                            // 处理消息
                            onMessage(fd, ctx.header, std::string(ctx.body.begin(), ctx.body.end()));

                            // 清空缓冲区，保留未处理的字节
                            memcpy(ctx.buffer, ctx.buffer + body_bytes_needed, ctx.buffer_pos - body_bytes_needed);
                            ctx.buffer_pos -= body_bytes_needed;

                            // 切换到读取协议头状态
                            ctx.state = ReadState::HEADER;
                            ctx.body.clear();
                        } else {
                            // 将未处理的字节复制到消息体
                            memcpy(ctx.body.data() + ctx.body.size(), ctx.buffer, available_bytes);
                            ctx.body.resize(ctx.body.size() + available_bytes);

                            // 清空缓冲区
                            ctx.buffer_pos = 0;

                            // 需要更多数据才能完成消息体
                            break;
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error processing client data: " << e.what() << std::endl;
                    close(fd);
                    client_contexts.erase(fd);
                    break;
                }
            }
        }
    });
}
/*
void handleClientRead(int fd, uint32_t events) {

    thread_pool.commit([fd]() {
        char buffer[BUFSIZ];
        std::string full_message;

        while (true) {
            int bytesRead = read(fd, buffer, sizeof(buffer) - 1);
            if (bytesRead <= 0) {
                if (bytesRead == 0) {
                    std::cout << "Client disconnected." << std::endl;
                    if (online_fd.find(fd) != online_fd.end()) {
                        online_name.erase(online_fd[fd]);
                        online_fd.erase(fd);
                    }
                    close(fd);
                } else if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    std::cerr << "Read error: " << strerror(errno) << std::endl;
                }
                break;
            }
            buffer[bytesRead] = '\0';
            full_message.append(buffer, bytesRead);
        }

        if (full_message.empty()) {
            return;
        }

        // 如果是第一次接收消息，认为是客户端名字
        if (online_fd.find(fd) == online_fd.end()) {
            std::cout << "Client name: " << full_message << std::endl;
            online_name[full_message] = fd;
            online_fd[fd] = full_message;
        } else {
            // 按行分割消息
            Message msg = Message::deserialize(full_message);
            std::cout << "Received from client: " << full_message << std::endl;
            std::cout << "消息内容：" << msg.get_content() << std::endl;
            send(online_name[msg.get_recv()], full_message.c_str(), full_message.length(), 0);
        }
    });
}
    */
/*
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

                //建立双向映射，快速查找和删除
                online_name[result] = fd;
                online_fd[fd] = result;
               std::cout<<result<<"   "<<fd<<std::endl;
            }else{
                //sendToClient(tmp);
                Message msg = Message::deserialize(tmp);
                std::cout << "Received from client: " << tmp << std::endl;
                std::cout << "消息内容：" << msg.get_content() << std::endl;
                send(online_name[msg.get_recv()], tmp.c_str(), tmp.length(), 0);
            }
                
            
            // 处理完数据，并转发给其他客户端
            //sendToClient(buffer);

            //write(fd, "Message received", 17);
        } else if (bytesRead == 0) {
            std::cout << "Client disconnected." << std::endl;
            //在线列表删除该套接字
            online_name.erase(online_fd[fd]);
            online_fd.erase(fd);
            close(fd); // 客户端断开连接，关闭文件描述符
        } else {
            std::cerr << "Read error: " << strerror(errno) << std::endl;
        }
    });
}

*/

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

    //获取用户名字，
    //const char* request_name = "Please send your name"; 
    //write(client_fd, request_name, strlen(request_name));
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
