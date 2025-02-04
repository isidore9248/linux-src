#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

void *client_receive(void *arg);
void *client_send(void *arg);

int client_init(struct server_info *info, const char *server_ip, int server_port)
{
    info->client_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (info->client_sockfd == -1)
    {
        perror("socket error");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    int pton_ret = inet_pton(AF_INET, server_ip, &server_addr.sin_addr.s_addr);
    if (pton_ret <= 0)
    {
        printf("inet_pton error");
        close(info->client_sockfd);
        return -1;
    }

    int connect_ret = connect(info->client_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (connect_ret == -1)
    {
        printf("connect error\n");
        close(info->client_sockfd);
        return -1;
    }

    printf("connect to server %s:%d\n", server_ip, server_port);
    return 0;
}

int client_start(struct server_info *info)
{
    pthread_create(&info->recv_thread, NULL, client_receive, &info->client_sockfd);
    pthread_create(&info->send_thread, NULL, client_send, &info->client_sockfd);

    return 0;
}

void client_close(struct server_info *info)
{
    close(info->client_sockfd);
    pthread_cancel(info->recv_thread);
    pthread_cancel(info->send_thread);
    pthread_join(info->recv_thread, NULL);
    pthread_join(info->send_thread, NULL);
}

void *client_receive(void *arg)
{
    int *sockfd = (int *)arg;
    char recv_buf[BUFFER_SIZE];

    while (1)
    {
        ssize_t recv_ret = recv(*sockfd, recv_buf, sizeof(recv_buf), 0);
        if (recv_ret == -1)
        {
            printf("recv error");
            continue;
        }
        else if (recv_ret == 0)
        {
            printf("server close\n");
            return NULL;
        }
        else
        {
            recv_buf[recv_ret] = '\0';
            printf("recv from server: %s\n", recv_buf);
            printf("Enter message: \n");
        }
    }
    return NULL;
}

void *client_send(void *arg)
{
    int *sockfd = (int *)arg;
    char send_buf[BUFFER_SIZE];

    while (1)
    {
        printf("Enter message: ");
        fgets(send_buf, BUFFER_SIZE, stdin);
        send(*sockfd, send_buf, strlen(send_buf), 0);
    }
    return NULL;
}