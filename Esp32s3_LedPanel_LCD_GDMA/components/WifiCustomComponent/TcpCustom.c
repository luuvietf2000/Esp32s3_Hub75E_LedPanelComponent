#include "TcpCustom.h"
#include "QueueMessageWifi.h"
#include "esp_log.h"
#include <stdint.h>
#include "MessageComponent.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "portmacro.h"

#define TAG_TCP_SERVER																		"Tcp Server"
#define TAG_TCP_CLIENT																		"Tcp Client"

static uint8_t headerSend[HEADER_WIFI_MESSAGE];
static uint8_t headerReceive[HEADER_WIFI_MESSAGE];
static uint32_t mId;

TcpClientReceiveStateEnum TcpCustomReceiveMsg(int socket, WifiMessage *msg){
	ssize_t headerBytes = recv(socket, headerReceive, HEADER_WIFI_MESSAGE, TCP_BLOCK);
	if(headerBytes == EAI_AGAIN || headerBytes == ETIMEDOUT){
		ESP_LOGE(TAG_TCP_CLIENT, "Client timeout!");
		return TCP_CLIENT_TIMEOUT;
	} else if(headerBytes == 0){
		ESP_LOGE(TAG_TCP_CLIENT, "Client disconnect!");
		return TCP_CLIENT_DISCONNECT;
	}
	HeaderFieldMessage headerMsg;
	GetFieldInMessage(headerReceive, &headerMsg);
	if(headerMsg.code != mId){
		ESP_LOGE(TAG_TCP_CLIENT, "Header invalid!");
		return TCP_CLIENT_HEADER_INVALID;
	}
	uint32_t contentSize = GetUlongInMessage(headerReceive);
	if(contentSize == 0){
		ESP_LOGE(TAG_TCP_CLIENT, "Size Message invalid!");
		return TCP_CLIENT_MSG_LENGTH_INVALID;
	}
	int receive = 0;
	uint32_t dataReceiveSize = 0;
	while(receive < contentSize){
		dataReceiveSize = recv(socket, msg->buffer + receive, contentSize, TCP_BLOCK);
		if(dataReceiveSize < 1)
			return TCP_CLIENT_MISSING_DATA;
		receive += dataReceiveSize;
	}
	msg->length = contentSize;
	return TCP_SERVER_RECEIVE_MSG_OK;
}

void TcpCustomSetId(uint32_t id){
	mId = id;
}

BaseType_t TcpCustomSendMessage(WifiMessage *msg){
	uint32_t headerBytes = AddUlongToMessage(headerSend, mId, msg->length);
	uint32_t bytesSent = send(msg->client, headerSend, headerBytes, TCP_BLOCK);
	if(headerBytes != bytesSent){
		ESP_LOGE(TAG_TCP_SERVER, "Send header fail!");
		return pdFALSE;
	}
	bytesSent = 0;
	uint32_t byte = 0;
	uint32_t blockSize;
	while(bytesSent < msg->length){
		blockSize = (msg->length - bytesSent) > TCP_MSG_DATA_MAX ? TCP_MSG_DATA_MAX : (msg->length - bytesSent);
		byte = send(msg->client, msg->buffer + bytesSent, blockSize, TCP_BLOCK);
		if(byte < 1)
			return pdFALSE;
		bytesSent += byte;
	}
	return pdTRUE;
}

TcpCustomInitState TcpCustomInit(int *server, uint32_t port){
	struct sockaddr_in server_addr;

	*server = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (*server < 0) {
		ESP_LOGE(TAG_TCP_SERVER, "Create fail");
	    return TCP_CUSTOM_INIT_FAIL_CAUSE_SOCKET_CREATE_FAIL;
	}
	
	int opt = 1;
	setsockopt(*server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);
	
	// Bind
	if (bind(*server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
	    close(*server);
	   	ESP_LOGE(TAG_TCP_SERVER, "Bind fail");
	    return TCP_CUSTOM_INIT_FAIL_CAUSE_BIND_FAIL;
	}
	
	// Listen
	if (listen(*server, 1) < 0) {
	    close(*server);
	    ESP_LOGE(TAG_TCP_SERVER, "Lisen fail");
	    return TCP_CUSTOM_INIT_FAIL_CAUSE_LISTEN_FAIL;
	}
	
	return TCP_CUSTOM_INIT_OK;
}

TcpCustomAcceptState TcpCustomAccept(int *server, struct sockaddr_storage  *client, socklen_t *length, int *clientSocket){
	*clientSocket = accept(*server, (struct sockaddr *) client, length);
	if (*clientSocket < 0) {
		ESP_LOGE(TAG_TCP_SERVER, "Accept fail");
	    return TCP_CUSTOM_ACCEPT_FAIL;
	}

	return TCP_CUSTOM_ACCEPT_OK;
}

void TcpCustomSetTimeoutReadPacket(int *clientSocket){
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 20000; // 20ms
	
	setsockopt(*clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

void TcpServerSetKeepAlive(int *socket, int *keepAlive, int *keepIdle, int *keepInterval, int *keepCount){
    setsockopt(*socket, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
    setsockopt(*socket, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
    setsockopt(*socket, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
    setsockopt(*socket, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
}