#pragma once

typedef enum
{
	COMM_TCP,  // TCP通信方式
	COMM_UDP   // UDP通信方式
} CommType;

//外部接口
void select_init(int port, CommType type, int backlog);
void server_start(void);
void server_close(void);

//内部调试接口
void socket_init(int port, CommType type, int backlog);
int socket_get_serverfd(void);
