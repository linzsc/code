#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
int main() {
    int pipefd[2]; // 创建管道
    char buffer[1024];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // 子进程
        close(pipefd[0]); // 关闭读端
        dup2(pipefd[1], STDOUT_FILENO); // 重定向标准输出到管道     //这里STDOUT_FILENO相当与写端的副本，可以直接关闭写端，但依旧可以通过stdout输出到管道 
        close(pipefd[1]); // 关闭写端
        execlp("ls", "ls", "-l", NULL); // 执行 ls 命令
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        // 父进程
        close(pipefd[1]); // 关闭写端
        read(pipefd[0], buffer, sizeof(buffer)); // 读取子进程输出
        printf("ls 命令输出:\n%s", buffer);
        close(pipefd[0]); // 关闭读端
        wait(NULL); // 等待子进程
    }

    return 0;
}
