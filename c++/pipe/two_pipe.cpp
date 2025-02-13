#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
int main() {
    int pipe1[2], pipe2[2]; // 两个管道
    pid_t pid;
    char buffer[128];

    // 创建两个管道
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // 创建子进程
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // 子进程
        close(pipe1[1]); // 关闭父->子 写端
        close(pipe2[0]); // 关闭子->父 读端

        read(pipe1[0], buffer, sizeof(buffer)); // 读取父进程数据
        printf("子进程收到: %s\n", buffer);

        char response[] = "Hello from child!";
        write(pipe2[1], response, strlen(response) + 1); // 发送数据给父进程

        close(pipe1[0]);
        close(pipe2[1]);
    } else {
        // 父进程
        close(pipe1[0]); // 关闭父->子 读端
        close(pipe2[1]); // 关闭子->父 写端

        char msg[] = "Hello from parent!";
        write(pipe1[1], msg, strlen(msg) + 1); // 发送数据给子进程

        read(pipe2[0], buffer, sizeof(buffer)); // 读取子进程数据
        printf("父进程收到: %s\n", buffer);

        close(pipe1[1]);
        close(pipe2[0]);
        wait(NULL); // 等待子进程结束
    }

    return 0;
}
