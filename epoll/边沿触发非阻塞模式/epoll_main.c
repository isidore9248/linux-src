#include "epoll_server.h"

const int SERVER_PORT = 8080;

/// <summary>
/// 服务器通信函数，处理客户端连接和请求。
/// </summary>
/// <param name="epoll_fd">epoll实例的文件描述符</param>
/// <param name="server_sockfd">服务器套接字文件描述符</param>
/// <param name="triggermode">触发模式（边沿触发或水平触发，搜索 enum TriggerMode）</param>
/// <param name="blockmode">
/// 阻塞模式（阻塞或非阻塞，搜索 enum BlockingMode） ！！仅在ET模式下生效！！
/// </param>
void server_communicate(int epoll_fd, int server_sockfd,
	enum TriggerMode triggermode, enum BlockingMode blockmode)
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
				if (blockmode == NONBLOCK && triggermode == ET)
				{
					int flag = fcntl(cfd, F_GETFL);
					flag |= O_NONBLOCK;
					fcntl(cfd, F_SETFL, flag);
					//设置完成之后, 读写都变成了非阻塞模式
				}
				// 将新的客户端文件描述符添加到epoll实例中
				add_fd_to_epoll(epoll_fd, cfd, EPOLLIN, triggermode);
			}
			else//与客户端通信
			{
				// 处理客户端请求
				handle_client(epoll_fd, current_fd, triggermode);
			}
		}
	}
}

int main(int argc, const char* argv[])
{
	int server_sockfd = -1;
	struct sockaddr_in serv_addr;
	// 设置服务器套接字
	server_socket_init(&server_sockfd, &serv_addr, SERVER_PORT);

	// epoll相关初始化
	int epoll_fd = -1;
	server_epoll_init(&epoll_fd, server_sockfd, EPOLLIN, 100, ET);

	// 服务器通信
	server_communicate(epoll_fd, server_sockfd, ET, NONBLOCK);

	close(server_sockfd);

	return 0;
}