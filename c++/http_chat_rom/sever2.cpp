#include "server.h"
#include "EpollReactor.h"
#include "http_header.h"
/*
前端先发送 注册、登录http请求，
但后端核实身份信息后，返回成功的响应
然后，前端发送websocket升级请求
后端生成一个唯一的session_id，并返回给前端
前端将session_id保存到cookie中
前端每次发送消息时，都带上session_id
后端收到消息后，根据session_id验证客户端身份
根据message中的type字段，选择群发和私聊

*/

// 定义回调函数类型
typedef void (*CallbackFunction)(int fd, uint32_t events);

// 定义客户端上下文
struct ClientContext {
    std::string buffer;
    int buffer_pos;
    bool is_websocket; // 标识是否是WebSocket连接
    bool is_logged_in; // 标识是否已登录

    ClientContext() : buffer_pos(0), is_websocket(false) {}
};

// Epoll Reactor 服务器

EpollReactor server;

// 全局变量
std::unordered_map<int, ClientContext> client_contexts;

// 工具函数：生成WebSocket Sec-WebSocket-Accept 响应
std::string generate_accept_key(const std::string& key) {
    std::string guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = key + guid;
    unsigned char hash[20];
    // 使用 SHA-1 进行哈希
    // 这里需要实现SHA-1哈希算法，或使用第三方库
    // 为了简化，这里直接返回一个示例值
    return "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";
}

// 构造 HTTP 响应
/*
std::string createHttpResponse(const std::string status_code, const std::string& content_type, const std::string& body) {
    std::ostringstream response_stream;
    response_stream << "HTTP/1.1 " << status_code << "\r\n";
    response_stream << "Content-Type: " << content_type << "\r\n";
    response_stream << "Content-Length: " << body.length() << "\r\n\r\n";
    response_stream << body;
    return response_stream.str();
}
*/

// 发送 HTTP 响应
void sendHttpResponse(int fd, HttpStatus status_code, const std::string& content_type, const std::string& body) {
    std::string response = HttpParser::createHttpResponse(status_code, content_type, body);
    std::cout << "Sending HTTP response: " << response << std::endl;
    send(fd, response.c_str(), response.length(), 0);
}

// 处理用户注册
void user_register(int fd, std::string ctx_body) {

    SimpleJSONParser parser(ctx_body);
    std::map<std::string, std::string> user_info=parser.parse();
    std::string username=user_info["username"],password=user_info["password"] ;
    std::cout << "register request: " << username << ", " << password << std::endl;
    //std::string username="linz",password="0128";
    // 模拟注册进入数据库
    if(user_db.find(username)==user_db.end()){
        user_db[username] = password;
        sendHttpResponse(fd, HttpStatus::OK, "application/json", "注册成功！");
    }
    else{
        sendHttpResponse(fd,HttpStatus::NOT_FOUND, "application/json", "{\"status\":\"这个id已经注册过了！\"}");
    }
 
    std::cout << "Registered user: " << username << ", password: " << password << std::endl;

}

// 处理用户登录
void user_login(int fd, std::string ctx_body)
{
    //ctx_body是json格式，使用SimpleJSONParser解析
    SimpleJSONParser parser(ctx_body);
    std::map<std::string, std::string> user_info=parser.parse();

    std::string username=user_info["username"],password=user_info["password"] ;
    std::cout << "Login request: " << username << ", " << password << std::endl;
    // 模拟登录验证
    if (user_db.find(username) != user_db.end() && user_db[username] == password)
    {
        //用户在数据库内，且登录加入到在线列表中，
        online_name[username] = fd;
        online_fd[fd] = username;
        std::cout<<username<<"成功登录\n";
        sendHttpResponse(fd,  HttpStatus::OK, "text/html", "Login successful");
    }
    else
    {
        sendHttpResponse(fd,  HttpStatus::NOT_FOUND, "text/html", "Unauthorized");
    }
}

// 处理 WebSocket 消息
void handleWebSocketMessage(int fd, std::string message) {//广播消息
    // 这里假设消息是 JSON 格式
    std::cout << "Received WebSocket message: " << message << std::endl;
    // 广播消息
    for (auto it = online_fd.begin(); it != online_fd.end(); ++it) {
        int client_fd = it->first;
        if (client_fd != fd) {
            send(client_fd, message.c_str(), message.length(), 0);
        }
    }
}

void onCtx_body(int fd, std::string message) {
    ClientContext &ctx = client_contexts[fd];
    ctx.buffer[ctx.buffer_pos] = '\0'; // Ensure null-terminated string
    std::cout<<"Received message: \n"<<message<<std::endl;
    if (!ctx.is_websocket) {
        // 处理 HTTP 请求
        size_t end_header = message.find("\r\n\r\n");
        if (end_header != std::string::npos) {
            std::string request_line = message.substr(0, end_header);
            std::regex request_regex("^(GET|POST) (\\S+) HTTP/1\\.1");
            std::smatch match;
            if (std::regex_search(request_line, match, request_regex)) {
                std::string method = match[1];
                std::string path = match[2];
                std::string body = message.substr(end_header + 4, message.length() - (end_header + 4));

                if (path.find("/ws") == 0) {
                    // WebSocket 升级请求
                    std::regex accept_regex("Sec-WebSocket-Key: (\\S+)");
                    std::smatch accept_match;
                    std::string key;
                    if (std::regex_search(body, accept_match, accept_regex)) {
                        key = accept_match[1];
                    }
                    //std::string accept = generate_accept_key(key);

                    std::string response = "HTTP/1.1 101 \r\n"
                                           "Upgrade: websocket\r\n"
                                           "Connection: Upgrade\r\n";
                                           //"Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
                    send(fd, response.c_str(), response.length(), 0);
                    ctx.is_websocket = true;
                } else if (path.find("/login") == 0) {
                    // 登录请求
                    user_login(fd, body);
                    ctx.is_websocket = true;
                } else if (path.find("/register") == 0) {
                    // 注册请求
                    user_register(fd, body);
                } else {
                    sendHttpResponse(fd, HttpStatus::NOT_FOUND, "text/html", "Not Found");
                }
            } else {
                sendHttpResponse(fd, HttpStatus::NOT_FOUND, "text/html", "Bad Request");
            }
            ctx.buffer_pos = 0; // 清空缓冲区
        }
    } else {
        // 处理 WebSocket 消息
        handleWebSocketMessage(fd, message);
        //ctx.buffer_pos = 0;
    }
}

void handleClientRead(int fd, uint32_t events) {
    if (client_contexts.find(fd) == client_contexts.end()) {
        client_contexts[fd] = ClientContext();
    }
    
    ClientContext &ctx = client_contexts[fd];
    char chunk[4096];
    ssize_t bytesRead;
    
    while ((bytesRead = read(fd, chunk, sizeof(chunk))) > 0) {
        ctx.buffer.append(chunk, bytesRead);  // 追加数据到缓冲区

        while (!ctx.buffer.empty()) {
            if (!ctx.is_websocket) {
                // 处理 HTTP 请求
                size_t end_header = ctx.buffer.find("\r\n\r\n");
                if (end_header == std::string::npos) {
                    break;  // 还没接收完整 HTTP 头部
                }

                std::string header = ctx.buffer.substr(0, end_header + 4);
                size_t content_length_pos = header.find("Content-Length: ");
                size_t content_length = 0;
                if (content_length_pos != std::string::npos) {
                    content_length = std::stoi(header.substr(content_length_pos + 16));
                }
                
                size_t total_size = end_header + 4 + content_length;
                if (ctx.buffer.size() < total_size) {
                    break; // HTTP 请求数据还未完全到达
                }
                
                std::string full_request = ctx.buffer.substr(0, total_size);
                ctx.buffer.erase(0, total_size); // 清理已处理部分
                
                onCtx_body(fd, full_request);
            } else {
                // 处理 WebSocket 消息
                if (ctx.buffer.size() < 2) {
                    break; // WebSocket 帧最小长度
                }
                
                size_t payload_length = ctx.buffer[1] & 0x7F;
                size_t header_length = 2;
                
                if (payload_length == 126) {
                    if (ctx.buffer.size() < 4) break;
                    payload_length = (ctx.buffer[2] << 8) | ctx.buffer[3];
                    header_length = 4;
                } else if (payload_length == 127) {
                    if (ctx.buffer.size() < 10) break;
                    payload_length = 0;
                    for (int i = 2; i < 10; i++) {
                        payload_length = (payload_length << 8) | ctx.buffer[i];
                    }
                    header_length = 10;
                }
                
                size_t total_frame_size = header_length + payload_length;
                if (ctx.buffer.size() < total_frame_size) {
                    break; // WebSocket 消息未完整
                }
                
                std::string ws_message = ctx.buffer.substr(header_length, payload_length);
                ctx.buffer.erase(0, total_frame_size); // 清理已处理部分
                
                handleWebSocketMessage(fd, ws_message);
            }
        }
    }
    
    if (bytesRead == 0) {
        std::cout << "Client disconnected." << std::endl;
        client_contexts.erase(fd);
        close(fd);
    } else if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        std::cerr << "Read error: " << strerror(errno) << std::endl;
        client_contexts.erase(fd);
        close(fd);
    }
}

/*
//读取客户端数据
void handleClientRead(int fd, uint32_t events) {
    if (client_contexts.find(fd) == client_contexts.end()) {
        client_contexts[fd] = ClientContext();
    }

    ClientContext& ctx = client_contexts[fd];

    while (true) {
        char chunk[4096];
        ssize_t bytesRead = read(fd, chunk, sizeof(chunk));
        if (bytesRead <= 0) {
            if (bytesRead == 0) {
                std::cout << "Client disconnected." << std::endl;
                client_contexts.erase(fd);
                close(fd);
                return;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 缓冲区暂时没有数据，退出循环
                    break;
                } else {
                    std::cerr << "Read error: " << strerror(errno) << std::endl;
                    client_contexts.erase(fd);
                    close(fd);
                    return;
                }
            }
        }

        // 累积到缓冲区
        memcpy(ctx.buffer + ctx.buffer_pos, chunk, bytesRead);
        ctx.buffer_pos += bytesRead;

        // 处理缓冲区中的数据
        while (ctx.buffer_pos > 0) {
            // 将缓冲区中的数据传递给 onMessage 进行处理
            std::string message(reinterpret_cast<char*>(ctx.buffer), ctx.buffer_pos);
            //处理消息
            onCtx_body(fd, message);

            // 清空缓冲区
            ctx.buffer_pos = 0;
        }
    }
}

*/
// 处理新连接
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
    server.addFd(client_fd, EPOLLIN | EPOLLET, handleClientRead);
}

// 处理标准输入
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
    server_addr.sin_port = htons(8080); // 监听端口改为 8080
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
    std::cout << "Server listening on port 8080" << std::endl;

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