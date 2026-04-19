#ifndef COMPONENTS_WIFICUSTOMCOMPONENT_INCLUDE_TCPCUSTOM_H_
#define COMPONENTS_WIFICUSTOMCOMPONENT_INCLUDE_TCPCUSTOM_H_

#include "QueueMessageWifi.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define TCP_BLOCK																			0
#define TCP_MSG_DATA_MAX																	1024
#define HEADER_WIFI_MESSAGE																	(LENGTH_FIELD_MESSAGE_SIZE + CODE_FIELD_MESSAGE_SIZE + ULONG_MESSAGE_TYPE)

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

typedef enum{
	TCP_CLIENT_DISCONNECT,
	TCP_CLIENT_TIMEOUT,
	TCP_CLIENT_HEADER_INVALID,
	TCP_CLIENT_MSG_LENGTH_INVALID,
	TCP_CLIENT_MISSING_DATA,
	TCP_SERVER_RECEIVE_MSG_OK
} TcpClientReceiveStateEnum;

TcpClientReceiveStateEnum TcpCustomReceiveMsg(int socket, WifiMessage *msg);
BaseType_t TcpCustomSendMessage(WifiMessage *msg);
void TcpCustomSetId(uint32_t id);
TcpCustomInitState TcpCustomInit(int *server, uint32_t port);
TcpCustomAcceptState TcpCustomAccept(int *server, struct sockaddr_storage  *client, socklen_t *length, int *clientSocket);
void TcpCustomSetTimeoutReadPacket(int *clientSocket);
void TcpServerSetKeepAlive(int *socket, int *keepAlive, int *keepIdle, int *keepInterval, int *keepCount);

#endif /* COMPONENTS_WIFICUSTOMCOMPONENT_INCLUDE_TCPCUSTOM_H_ */