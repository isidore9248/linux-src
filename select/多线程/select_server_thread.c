#include "select_server_thread.h"
#include <malloc.h>

static int server_sockfd;
pthread_mutex_t mutex;

void socket_init(int port, CommType type, int backlog)
{
	if (type == COMM_TCP) { server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); }
	else if (type == COMM_UDP) { server_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); }

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)port);
	addr.sin_addr.s_addr = INADDR_ANY;
	bind(server_sockfd, (struct sockaddr*)&addr, sizeof(addr));
	listen(server_sockfd, backlog);
}

void select_pthread_init(int port, CommType type, int backlog)
{
	socket_init(port, type, backlog);
	pthread_mutex_init(&mutex, NULL);
}

int socket_get_serverfd(void) { return server_sockfd; }

/// <summary>
/// rd-read set-集合
/// </summary>
/// <param name=""></param>
void server_pthread_start(void)
{
	int lfd = socket_get_serverfd();
	int maxfd = lfd;

	//清除集合，使其不包含任何文件描述符
	//确保select调用时，没有文件描述符被设置
	fd_set rdset; FD_ZERO(&rdset);
	//设置文件描述符添加到文件描述符集合中
	//以便 select 函数可以监视该文件描述符的状态。
	FD_SET(lfd, &rdset);

	/// 由于select函数会修改rd，所以设置两个fd_set类型变量，以便对集合进行操作
	while (1)
	{
		pthread_mutex_lock(&mutex);
		fd_set rdtemp = rdset;
		pthread_mutex_unlock(&mutex);

		/// int nfds				需要监视的最大文件描述符+1
		/// fd_set *readfds			查看监视的文件描述符集合 是否数据可读
		///						NULL 表示不监视任何文件描述符的读状态。
		/// fd_set *writefds		查看监视的文件描述符集合 是否可以写入
		///						NULL 表示不监视任何文件描述符的写状态
		/// fd_set *exceptfds		查看监视的文件描述符集合 是否有异常情况发生
		///						 NULL 表示不监视任何文件描述符的异常状态。
		/// struct timeval *timeout	指定 select 函数的超时时间
		///						NULL 表示 select 函数将无限期地阻塞，直到有文件描述符变为就绪。
		/// ret						返回值 num 表示有多少个文件描述符变为就绪，可以进行相应的读操作。
		select(maxfd + 1, &rdtemp, NULL, NULL, NULL);

		//检查指定的文件描述符是否在文件描述符集合中被设置
		//更新maxfd
		if (FD_ISSET(lfd, &rdtemp))
		{
			struct client_info* accept_arg = (struct client_info*)malloc(sizeof(struct client_info));
			accept_arg->server_sockfd = lfd;
			accept_arg->max_fd = &maxfd;
			accept_arg->fd_set = &rdset;
			pthread_create(&(accept_arg->accept_tid), NULL, server_accept, accept_arg);
			pthread_detach(accept_arg->accept_tid);
		}

		for (int i = 0; i < maxfd + 1; ++i)
		{
			if (i != lfd && FD_ISSET(i, &rdtemp))
			{
				struct client_info* communicate_arg = (struct client_info*)malloc(sizeof(struct client_info));
				communicate_arg->server_sockfd = lfd;
				communicate_arg->fd_set = &rdset;
				communicate_arg->client_sockfd = i;
				pthread_create(&(communicate_arg->communicate_tid), NULL, server_communicate, communicate_arg);
				pthread_detach(communicate_arg->communicate_tid);
			}
		}
	}
}

void server_pthread_close(void)
{
	close(server_sockfd);
	pthread_mutex_destroy(&mutex);
}

void* server_accept(void* arg)
{
	struct client_info* clinfo = (struct client_info*)arg;

	struct sockaddr_in client_addr;
	socklen_t cliLen = sizeof(client_addr);
	int client_sockfd = accept(clinfo->server_sockfd, (struct sockaddr*)&client_addr, &cliLen);

	pthread_mutex_lock(&mutex);
	FD_SET(client_sockfd, clinfo->fd_set);
	*clinfo->max_fd = client_sockfd > *clinfo->max_fd ? client_sockfd : *clinfo->max_fd;
	pthread_mutex_unlock(&mutex);

	free(clinfo);
	return NULL;
}

void* server_communicate(void* arg)
{
	struct client_info* communicate_arg = (struct client_info*)arg;

	char buf[1024] = { 0 };
	int len = read(communicate_arg->client_sockfd, buf, sizeof(buf));
	if (len >= 0) { buf[len] = '\0'; }

	if (len > 0)
	{
		printf("客户端发送的数据是：%s\n", buf);
		write(communicate_arg->client_sockfd, buf, strlen(buf) + 1);
	}
	else if (len == 0)
	{
		printf("客户端关闭了连接...\n");

		pthread_mutex_lock(&mutex);
		FD_CLR(communicate_arg->client_sockfd, communicate_arg->fd_set);
		pthread_mutex_unlock(&mutex);

		close(communicate_arg->client_sockfd);
		free(communicate_arg); // 添加free
		return NULL;
	}
	else if (len < 0) { perror("read"); }
	free(communicate_arg); // 添加free
	return NULL;
}