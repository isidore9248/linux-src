#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>

#define BUFFER_SIZE 1024

struct server_info
{
    int client_sockfd;
    pthread_t send_thread;
    pthread_t recv_thread;
};

// 初始化客户端并连接到服务器
int client_init(struct server_info *info, const char *server_ip, int server_port);

// 启动发送和接收线程
int client_start(struct server_info *info);

// 关闭客户端连接
void client_close(struct server_info *info);

#endif // CLIENT_H