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
    