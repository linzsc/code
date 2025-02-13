#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

int main(int argc, char* argv[]) {
    std::cout << "成功调用 add.cpp!" << std::endl;
    
    // 获取当前进程 ID
    pid_t pid = getpid();
    std::cout << "子进程 PID: " << pid << std::endl;

    // 读取传入的 epoll 文件描述符
    int epoll_fd = -1;
    if (argc > 1) {
        epoll_fd = atoi(argv[1]);
        std::cout << "父进程传递的 epoll_fd: " << epoll_fd << std::endl;
    }

    // 获取当前进程的文件描述符列表
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/fd/", pid);

    std::cout << "子进程的文件描述符列表：" << std::endl;

    DIR* dir = opendir(path);
    if (!dir) {
        std::cerr << "无法打开目录: " << path << std::endl;
        return 1;
    }

    struct dirent* entry;
    bool found = false;
    while ((entry = readdir(dir)) != nullptr) {
        int fd = atoi(entry->d_name);
        std::cout << "文件描述符: " << fd << std::endl;
        if (fd == epoll_fd) {
            found = true;
        }
    }

    closedir(dir);

    if (found) {
        std::cerr << "❌ 子进程继承了 epoll_fd，这是一个错误！" << std::endl;
    } else {
        std::cout << "✅ 子进程没有继承 epoll_fd，行为正确。" << std::endl;
    }

    return 0;
}
