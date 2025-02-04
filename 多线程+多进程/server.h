#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024
#define MAX_CONNECT 5

struct client_info
{
    int client_sockfd;
    struct sockaddr_in client_addr;
    pthread_t recv_thread;
    pthread_t send_thread;
};

// 初始化服务器并绑定端口
int server_init(int *server_sockfd, int server_port);

// 启动服务器，等待客户端连接
void server_start(int server_sockfd);

// 关闭服务器
void server_close(int server_sockfd);

#endif // SERVER_H