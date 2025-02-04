#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <pthread.h>

typedef enum
{
	COMM_TCP,  // TCP通信方式
	COMM_UDP   // UDP通信方式
} CommType;

struct client_info
{
	int server_sockfd;
	int* max_fd;
	int client_sockfd;
	fd_set* fd_set;
	pthread_t accept_tid;
	pthread_t communicate_tid;
};

//外部接口
void select_pthread_init(int port, CommType type, int backlog);
void server_pthread_start(void);
void server_pthread_close(void);

//线程函数
void* server_accept(void* arg);
void* server_communicate(void* arg);