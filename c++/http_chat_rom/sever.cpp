#include "server.h"


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



std::unordered_map<int, ClientContext> client_contexts;


std::string create_response(int fd, const std::string& status_code, const std::string& content_type, const std::string& body){
    ProtocolHeader  header;
    header.version=1;
    if(status_code=="100"){
        header.setMessageType(ProtocolHeader::MsgType::HTTP_RESPONSE);
    }
    else if(status_code=="101"){
        header.setMessageType(ProtocolHeader::MsgType::CHAT_RESPONSE);
    }

    std::string response = "HTTP/1.1 " + status_code + "\r\n"; // 状态行
    response += "Content-Type: " + content_type + "\r\n";        // 内容类型                                          // 空行，表示响应头结束
    response += "Content-Length: "+std::to_string(body.size())+"\r\n\r\n";
    response += body;
    header.setBodyLength(response.size());
    response=header.serialize()+response;

    std::cout<<"<create_response: >\n"+response<<std::endl;
    return response;
} 

void sendToClient(int fd,std::string & message) {//后续还需要区别群聊和私聊，目前只是私聊
    std::cout<<"\n\nsendToClient: "<<std::endl;
    Message msg=Message::deserialize(message);
    std::string response=create_response(fd,"101","text/html",message);

    if(online_fd[fd]!=msg.get_sender()||online_fd.find(fd)==online_fd.end()){
        std::cout<<msg.get_sender()<<"该发送方为非法用户！！不在在线列表中"<<std::endl;
        return;
    }
    if(online_fd.find(online_name[msg.get_recv()])==online_fd.end()){
        std::cout<<msg.get_recv()<<"该接受方为非法用户！！不在在线列表中"<<std::endl;
        return;
    }
    
    send(online_name[msg.get_recv()],response.c_str(), response.length(), 0);

}

void sendHttpResponse(int fd, const std::string& status_code, const std::string& content_type, const std::string& body) {
    // 构造 HTTP 响应头
    ProtocolHeader  header;
    header.version=1;
    if(status_code=="100"){
        header.setMessageType(ProtocolHeader::MsgType::HTTP_RESPONSE);
    }
    else if(status_code=="101"){
        header.setMessageType(ProtocolHeader::MsgType::CHAT_RESPONSE);
    }

    std::string response = "HTTP/1.1 " + status_code + "\r\n"; // 状态行
    response += "Content-Type: " + content_type + "\r\n";        // 内容类型                                          // 空行，表示响应头结束

    // 构造 HTTP 响应体
    response += body;
    header.setBodyLength(response.size());
    response=header.serialize()+response;
    std::cout<<"sendHttpResponse : "<<response<<std::endl;
    // 将响应发送到客户端
    if (send(fd, response.c_str(), response.size(), 0) < 0) {
        std::cerr << "Failed to send HTTP response." << std::endl;
    }
}


void broadcastMessage(int fd,std::string message){
    Message msg=Message::deserialize(message);
    std::string response=create_response(fd,"101","text/html",message);
    for(auto it=online_fd.begin();it!=online_fd.end();it++){
        if(it->first==fd){
            continue;
        }
        send(it->first, response.c_str(), response.length(), 0);
    }
}


void user_register(int fd, std::string ctx_body) {

    SimpleJSONParser parser(ctx_body);
    std::map<std::string, std::string> user_info=parser.parse();
    std::string username=user_info["username"],password=user_info["password"] ;
    std::cout << "Login request: " << username << ", " << password << std::endl;
    //std::string username="linz",password="0128";
    // 模拟注册进入数据库
    if(user_db.find(username)==user_db.end()){
        user_db[username] = password;
    }
    else{
        sendHttpResponse(fd, "400", "application/json", "{\"status\":\"这个id已经注册过了！\"}");
    }
 
    std::cout << "Registered user: " << username << ", password: " << password << std::endl;

    
}

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
        //sendHttpResponse(fd, "200", "text/html", "Login successful");
    }
    else
    {
        //sendHttpResponse(fd, "401", "text/html", "Unauthorized");
    }
}



void onMessage(int fd, ProtocolHeader ctx_header, std::string ctx_body) {
    ctx_header.print();
    std::cout << "body: " << ctx_body << std::endl;
    
    try {
        printf("status:%d\n", ctx_header.msg_type);
        switch (ctx_header.msg_type) {
            case ProtocolHeader::MsgType::HTTP_REQUEST: {
                // 解析 HTTP 请求
                size_t first_space = ctx_body.find(' ');
                size_t second_space = ctx_body.find(' ', first_space + 1);

                if (first_space != std::string::npos && second_space != std::string::npos) {
                    std::string request_method = ctx_body.substr(0, first_space);
                    std::string request_path = ctx_body.substr(first_space + 1, second_space - first_space - 1);

                    std::cout << "\n\n\n\n" << request_path << '\n' << std::endl;
                    // std::transform(request_path.begin(), request_path.end(), request_path.begin(), ::tolower);

                    // 提取 JSON 数据
                    std::string json_data;
                    size_t body_start = ctx_body.find("\r\n\r\n");
                    if (body_start != std::string::npos) {
                        body_start += 4; // 跳过 "\r\n\r\n"
                        json_data = ctx_body.substr(body_start, ctx_body.size() - body_start);
                    } else {
                        sendHttpResponse(fd, "400", "text/html", "Bad Request: Missing body");
                        return;
                    }

                    // 处理 HTTP 请求路径
                    if (request_path == "/login") {
                        user_login(fd, json_data);
                    } else if (request_path == "/register") {
                        user_register(fd, json_data);
                    } else if (request_path.substr(0, 8) == "/static/") {
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
                std::cout << "message body: " << ctx_body << std::endl;
                sendToClient(fd, ctx_body);
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
    if (client_contexts.find(fd) == client_contexts.end()) {
        client_contexts[fd] = ClientContext();
    }

    ClientContext& ctx = client_contexts[fd];

    while (true) {
        char chunk[4096];
        ssize_t bytesRead = read(fd, chunk, sizeof(chunk));
        std::cout<<"chunk: "<<chunk<<std::endl;
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
        while (ctx.buffer_pos >= sizeof(ProtocolHeader)) {
            ProtocolHeader header;
            memcpy(&header, ctx.buffer, sizeof(header));

            // 校验魔数和版本
            if (header.magic != ProtocolHeader::MAGIC || header.version != 1) {
                std::cerr << "Invalid protocol header." << std::endl;
                client_contexts.erase(fd);
                close(fd);
                return;
            }

            size_t body_len = header.body_len;
            size_t message_size = sizeof(header) + body_len;

            // 检查缓冲区是否有足够的数据
            if (ctx.buffer_pos >= message_size) {
                std::string body(reinterpret_cast<char*>(ctx.buffer) + sizeof(header), body_len);
                onMessage(fd, header, body);

                // 移除已处理的数据
                size_t remaining = ctx.buffer_pos - message_size;
                if (remaining > 0) {
                    memmove(ctx.buffer, ctx.buffer + message_size, remaining);
                }
                ctx.buffer_pos = remaining;
            } else {
                // 数据不足，等待更多数据
                break;
            }
        }
    }
}

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
