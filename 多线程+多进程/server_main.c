//!编译命令：   gcc -o server main.c server.c -lpthread

#include "server.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    int server_sockfd;

    if (server_init(&server_sockfd, 9734) != 0)
    {
        return -1;
    }

    server_start(server_sockfd);

    // 等待用户输入以退出
    printf("Press Enter to exit...\n");
    getchar();

    server_close(server_sockfd);

    return 0;
}