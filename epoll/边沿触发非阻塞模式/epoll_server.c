#include "epoll_server.h"

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
 * @param mode 触发模式（边沿触发或水平触发，搜索 enum TriggerMode）
 */
void add_fd_to_epoll(int epoll_fd, int server_sockfd, uint32_t events, enum TriggerMode mode)
{
	struct epoll_event ev;
	ev.events = events;	if (mode == ET) { ev.events |= EPOLLET; }
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
 * @param mode 触发模式（边沿触发或水平触发，搜索 enum TriggerMode）
 */
void server_epoll_init(int* epoll_fd, int server_sockfd, uint32_t events, int max_events, enum TriggerMode mode)
{
	// 创建epoll实例
	*epoll_fd = epoll_create(max_events);
	if (*epoll_fd == -1) { perror("epoll_create"); exit(0); }

	enum TriggerMode flag = mode;
	add_fd_to_epoll(*epoll_fd, server_sockfd, EPOLLIN, flag);
}

/**
 * @brief 处理客户端连接
 *
 * @param epoll_fd epoll实例的文件描述符
 * @param curfd 当前客户端的文件描述符
 * @param mode 触发模式（边沿触发或水平触发，搜索 enum TriggerMode）
 */
void handle_client(int epoll_fd, int curfd, enum TriggerMode triggermode)
{
	if (triggermode == LT)
	{
		char buf[1024];
		memset(buf, 0, sizeof(buf));
		// 接收客户端数据
		int len = recv(curfd, buf, sizeof(buf), 0);
		printf("recv over\n");
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
	else if (triggermode == ET)
	{
		// 处理通信的文件描述符
				// 接收数据
		char buf[5];
		memset(buf, 0, sizeof(buf));
		// 循环读数据
		while (1)
		{
			int len = recv(curfd, buf, sizeof(buf), 0);
			if (len == 0)
			{
				// 非阻塞模式下和阻塞模式是一样的 => 判断对方是否断开连接
				printf("客户端断开了连接...\n");
				// 将这个文件描述符从epoll模型中删除
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, curfd, NULL);
				close(curfd);
				break;
			}
			else if (len > 0)
			{
				// 通信
				// 接收的数据打印到终端
				write(STDOUT_FILENO, buf, len);
				// 发送数据
				send(curfd, buf, len, 0);
			}
			else
			{
				// len == -1
				if (errno == EAGAIN || errno == EWOULDBLOCK) { printf("数据读完了...\n"); break; }
				else { perror("recv"); break; }
			}
		}
	}
}