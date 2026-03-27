#ifndef COMPONENTS_WIFICUSTOMCOMPONENT_INCLUDE_TCPCUSTOM_H_
#define COMPONENTS_WIFICUSTOMCOMPONENT_INCLUDE_TCPCUSTOM_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

typedef enum{
	TCP_CUSTOM_INIT_OK,
	TCP_CUSTOM_INIT_FAIL_CAUSE_SOCKET_CREATE_FAIL,
	TCP_CUSTOM_INIT_FAIL_CAUSE_BIND_FAIL,
	TCP_CUSTOM_INIT_FAIL_CAUSE_LISTEN_FAIL
} TcpCustomInitState;

typedef enum{
	TCP_CUSTOM_ACCEPT_OK,
	TCP_CUSTOM_ACCEPT_FAIL
} TcpCustomAcceptState;

TcpCustomInitState TcpCustomInit(int *server, uint32_t port);
TcpCustomAcceptState TcpCustomAccept(int *server, struct sockaddr_storage  *client, socklen_t *length, int *clientSocket);
void TcpCustomSetTimeoutReadPacket(int *clientSocket);
void TcpServerSetKeepAlive(int *socket, int *keepAlive, int *keepIdle, int *keepInterval, int *keepCount);

#endif /* COMPONENTS_WIFICUSTOMCOMPONENT_INCLUDE_TCPCUSTOM_H_ */