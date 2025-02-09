#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

/**
 * @brief 初始化服务器套接字
 *
 * @param lfd 指向监听文件描述符的指针
 * @param serv_addr 指向服务器地址结构的指针
 * @param port 端口号
 */
void server_socket_init(int* lfd, struct sockaddr_in* serv_addr, int port)
{
	// 创建套接字
	*lfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*lfd == -1) { perror("socket error"); exit(1); }

	memset(serv_addr, 0, sizeof(*serv_addr));
	serv_addr->sin_family = AF_INET;
	serv_addr->sin_port = htons(port);
	serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);

	int opt = 1;
	// 设置套接字选项
	setsockopt(*lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	int ret = bind(*lfd, (struct sockaddr*)serv_addr, sizeof(*serv_addr));
	if (ret == -1) { perror("bind error"); exit(1); }
	ret = listen(*lfd, 64);
	if (ret == -1) { perror("listen error"); exit(1); }
}

/**
 * @brief 将文件描述符添加到epoll实例中
 *
 * @param epoll_fd epoll实例的文件描述符
 * @param server_sockfd 服务器套接字文件描述符
 * @param events 事件类型
 */
void add_fd_to_epoll(int epoll_fd, int server_sockfd, uint32_t events)
{
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = server_sockfd;
	// 将文件描述符添加到epoll实例中
	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sockfd, &ev);
	if (ret == -1) { perror("epoll_ctl"); exit(0); }
}

/**
 * @brief 初始化epoll实例并添加服务器套接字
 *
 * @param epoll_fd 指向epoll实例文件描述符的指针
 * @param server_sockfd 服务器套接字文件描述符
 * @param events 事件类型
 * @param max_events 最大事件数
 */
void server_epoll_init(int* epoll_fd, int server_sockfd, uint32_t events, int max_events)
{
	// 创建epoll实例
	*epoll_fd = epoll_create(max_events);
	if (*epoll_fd == -1) { perror("epoll_create"); exit(0); }

	add_fd_to_epoll(*epoll_fd, server_sockfd, EPOLLIN);
}

/**
 * @brief 处理客户端连接
 *
 * @param epoll_fd epoll实例的文件描述符
 * @param curfd 当前客户端的文件描述符
 */
void handle_client(int epoll_fd, int curfd)
{
	char buf[1024];
	memset(buf, 0, sizeof(buf));
	// 接收客户端数据
	int len = recv(curfd, buf, sizeof(buf), 0);
	if (len > 0)
	{
		printf("客户端say: %s\n", buf);
		send(curfd, buf, len, 0);
	}
	else if (len == 0)
	{
		printf("客户端已经断开了连接\n");
		// 从epoll实例中删除文件描述符
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, curfd, NULL);
		close(curfd);
	}
	else { perror("recv"); exit(0); }
}

void server_communicate(int epoll_fd, int server_sockfd);

int main(int argc, char* argv[])
{
	int server_sockfd = -1;
	struct sockaddr_in serv_addr;
	// 设置服务器套接字
	server_socket_init(&server_sockfd, &serv_addr, 8080);

	// epoll相关初始化
	int epoll_fd = -1;
	server_epoll_init(&epoll_fd, server_sockfd, EPOLLIN, 100);

	// 服务器通信
	server_communicate(epoll_fd, server_sockfd);

	close(server_sockfd);
}

void server_communicate(int epoll_fd, int server_sockfd)
{
	struct epoll_event evs[1024];
	int size = sizeof(evs) / sizeof(struct epoll_event);

	while (1)
	{
		/// <summary>
		/// 阻塞并等待就绪的文件描述符
		/// </summary>
		/// <param name="evs">epoll事件结构体数组	！！地址！！	</param>
		/// <param name="size">epoll事件结构体数组大小</param>
		/// <returns>就绪的文件描述符数量</returns>
		int readyfd_num = epoll_wait(epoll_fd, evs, size, -1);
		for (int i = 0; i < readyfd_num; ++i)
		{
			int current_fd = evs[i].data.fd;
			if (current_fd == server_sockfd)
			{
				// 接受新的客户端连接
				int cfd = accept(current_fd, NULL, NULL);

				// 将新的客户端文件描述符添加到epoll实例中
				add_fd_to_epoll(epoll_fd, cfd, EPOLLIN);
			}
			else//与客户端通信
			{
				// 处理客户端请求
				handle_client(epoll_fd, current_fd);
			}
		}
	}
}