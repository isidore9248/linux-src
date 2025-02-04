#include "select_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>

static int server_sockfd;

void socket_init(int port, CommType type, int backlog)
{
	if (type == COMM_TCP) { server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); }
	else if (type == COMM_UDP) { server_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); }

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	bind(server_sockfd, (struct sockaddr*)&addr, sizeof(addr));
	listen(server_sockfd, backlog);
}

void select_init(int port, CommType type, int backlog)
{
	socket_init(port, type, backlog);
}

int socket_get_serverfd(void) { return server_sockfd; }

/// <summary>
/// rd-read set-集合
/// </summary>
/// <param name=""></param>
void server_start(void)
{
	int lfd = socket_get_serverfd();
	int maxfd = lfd;

	//清除集合，使其不包含任何文件描述符
	//确保select调用时，没有文件描述符被设置
	fd_set rdset; FD_ZERO(&rdset);
	//设置文件描述符添加到文件描述符集合中
	//以便 select 函数可以监视该文件描述符的状态。
	fd_set rdtemp;  FD_SET(lfd, &rdset);

	/// 由于select函数会修改rd，所以设置两个fd_set类型变量，以便对集合进行操作
	while (1)
	{
		rdtemp = rdset;
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
		int num = select(maxfd + 1, &rdtemp, NULL, NULL, NULL);

		//检查指定的文件描述符是否在文件描述符集合中被设置
		//更新maxfd
		if (FD_ISSET(lfd, &rdtemp))
		{
			struct sockaddr_in client_addr;
			socklen_t cliLen = sizeof(client_addr);
			int client_sockfd = accept(lfd, (struct sockaddr*)&client_addr, &cliLen);

			FD_SET(client_sockfd, &rdset);
			maxfd = client_sockfd > maxfd ? client_sockfd : maxfd;
		}

		for (int i = 0; i < maxfd + 1; ++i)
		{
			if (i != lfd && FD_ISSET(i, &rdtemp))
			{
				char buf[10] = { 0 };
				int len = read(i, buf, sizeof(buf));
				buf[len] = '\0';
				if (len == 0)
				{
					printf("客户端关闭了连接...\n");
					FD_CLR(i, &rdset);
					close(i);
				}
				else if (len > 0) { write(i, buf, strlen(buf) + 1); }
				else { perror("read"); }
			}
		}
	}
}

void server_close(void)
{
	close(server_sockfd);
}