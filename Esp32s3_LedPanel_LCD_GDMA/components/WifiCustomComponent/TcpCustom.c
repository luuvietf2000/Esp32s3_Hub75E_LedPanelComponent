#include "TcpCustom.h"
#include "esp_log.h"
#include <stdint.h>

#define TAG_TCP_SERVER																		"Tcp Server"
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