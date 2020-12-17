#define _GNU_SOURCE 1
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <termios.h>

#define BUFFER_SIZE 1024
#define PORT 22333 

//隐藏终端输入的退格字符
void hint_backspace()
{
    struct termios term;
    memset(&term, 0, sizeof(term));

    if (tcgetattr(STDIN_FILENO, &term) == -1)
        perror("tcgetattr error");

    term.c_cc[VERASE] = '\b';
    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) == -1)
        perror("tcsetattr error");

    return;
}

int main(int argc, char* argv[])
{
    int listenfd, connfd;
    char read_buf[BUFFER_SIZE];
    char write_buf[BUFFER_SIZE];
    char End_buf[BUFFER_SIZE] = "END\n";
    memset(read_buf, 0, sizeof(read_buf));
    memset(write_buf, 0, sizeof(write_buf));

    hint_backspace();

    struct sockaddr_in6 srvaddr, cliaddr;
    bzero(&srvaddr, sizeof(srvaddr));
    bzero(&cliaddr, sizeof(cliaddr));

    if ((listenfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        perror("server socket error");

    //地址复用
    int optval = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        perror("reuseaddr error");

    if ((connfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        perror("client socket error");

    if (argc < 2)
    {
        srvaddr.sin6_family = AF_INET6;
        srvaddr.sin6_port = htons(PORT);
        srvaddr.sin6_addr = in6addr_any;

        if (bind(listenfd, (struct sockaddr*)&srvaddr, sizeof(srvaddr)) == -1)
            perror("bind error");

        if (listen(listenfd, 5) == -1)
            perror("listen error");
    }
    else { //客户端连接
        const char* ip = argv[1];

        srvaddr.sin6_family = AF_INET6;
        inet_pton(AF_INET6, ip, &srvaddr.sin6_addr);
        srvaddr.sin6_port = htons(PORT);

        if (connect(connfd, (struct sockaddr*)&srvaddr, sizeof(srvaddr)) < 0)
        {
            printf("连接失败\n");
            close(connfd);
            return 1;
        }
        printf("\033[1;35;35m****************************************\033[0m\n");
        printf("\033[1;35;35m******已和对等方建立连接，开始通信******\033[0m\n");
        printf("\033[1;35;35m****************************************\033[0m\n");
    }

    struct pollfd fds[3];
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    fds[1].fd = listenfd;
    fds[1].events = POLLIN | POLLERR;
    fds[1].revents = 0;

    fds[2].fd = connfd;
    fds[2].events = POLLIN | POLLRDHUP | POLLERR;
    fds[2].revents = 0;

    while (1)
    {
        int ret = poll(fds, 3, -1);
        if (ret < 0)
        {
            printf("poll error");
            break;
        }

        //服务器等待连接
        if (fds[1].revents & POLLIN) {
            struct sockaddr_in peer_addr;
            socklen_t peer_addrlen = sizeof(peer_addr);
            connfd = accept(listenfd, (struct sockaddr*)&peer_addr, &peer_addrlen);

            if (connfd < 0) {
                printf("accept error");
                break;
            }

            printf("\033[1;35;35m****************************************\033[0m\n");
            printf("\033[1;35;35m******已和对等方建立连接，开始通信******\033[0m\n");
            printf("\033[1;35;35m****************************************\033[0m\n");
            fds[2].fd = connfd;
        }

        if (fds[2].revents & POLLRDHUP) {
            printf("\033[1;35;35m****************************************\033[0m\n");
            printf("\033[1;35;35m************对等方关闭了连接************\033[0m\n");
            printf("\033[1;35;35m****************************************\033[0m\n");
            break;
        }
        else if (fds[2].revents & POLLIN) { //接收对等方数据
            memset(read_buf, 0, sizeof(read_buf));
            recv(fds[2].fd, read_buf, BUFFER_SIZE-1, 0);
            if (strcmp(read_buf, End_buf) == 0) {
                printf("\033[1;35;35m****************************************\033[0m\n");
                printf("\033[1;35;35m************对等方关闭了连接************\033[0m\n");
                printf("\033[1;35;35m****************************************\033[0m\n");
                break;
            }

            printf("对等方:%s", read_buf);
        }

        if (fds[0].revents & POLLIN) {  //向对等方发送数据
            fgets(write_buf, BUFFER_SIZE-1, stdin);
            if (strcmp(write_buf, End_buf) == 0) {
                printf("\033[1;35;35m****************************************\033[0m\n");
                printf("\033[1;35;35m************你已经关闭了连接************\033[0m\n");
                printf("\033[1;35;35m****************************************\033[0m\n");
                break;
            }

            ret = send(connfd, write_buf, BUFFER_SIZE-1, 0);
            memset(write_buf, 0, sizeof(write_buf));
        }
    }

    close(listenfd);
    close(connfd);
    return 0;
}

