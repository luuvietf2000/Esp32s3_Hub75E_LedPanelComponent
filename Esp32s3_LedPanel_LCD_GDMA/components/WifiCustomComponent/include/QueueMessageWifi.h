#ifndef COMPONENTS_WIFICUSTOMCOMPONENT_QUEUEMESSAGEWIFI_H_
#define COMPONENTS_WIFICUSTOMCOMPONENT_QUEUEMESSAGEWIFI_H_

#include <stdint.h>
typedef enum{
	UDP_CUSTOM_INIT_OK,
	UDP_CUSTOM_INIT_FAIL_CAUSE_SOCKET_CREATE_FAIL,
	UDP_CUSTOM_INIT_FAIL_CAUSE_SOCKET_BIND_FAIL
} UdpCustomInitState;

typedef enum{
	QUEUE_WIFI_MESSAGE_INIT_OK,
	QUEUE_WIFI_MESSAGE_FAIL_CAUSE_ALLOCATION_WIFI_MESSAGE_FAIL,
	QUEUE_WIFI_MESSAGE_FAIL_CAUSE_ALLOCATION_BUFFER_FAIL
} QueueWifiMessageInit;

typedef enum{
	QUEUE_WIFI_MESSAGE_FULL,
	QUEUE_WIFI_MESSAGE_EMPTY,
	QUEUE_WIFI_MESSAGE_NORMAL
} QueueWifiMessageStateEnum;

typedef enum{
	QUEUE_WIFI_MESSAGE_PUSH_OK,
	QUEUE_WIFI_MESSAGE_PUSH_FAIL_CAUSE_QUEUE_FULL
} QueueWifiMessagePushState;

typedef enum{
	QUEUE_WIFI_MESSAGE_POP_OK,
	QUEUE_WIFI_MESSAGE_POP_FAIL_CAUSE_QUEUE_EMPTY
} QueueWifiMessagePopState;

typedef struct WifiMessage{
	int client;
	uint8_t *buffer;
	uint32_t length;
} WifiMessage;

typedef struct WifiMessageQueue{
	uint32_t rear;
	uint32_t size;
	uint32_t front;
	WifiMessage *head;
} WifiMessageQueue;

WifiMessage *PeekHeadQueueWifiMessage(WifiMessageQueue *queue);

WifiMessage *PeekTailQueueWifiMessage(WifiMessageQueue *queue);

QueueWifiMessagePopState QueueWifiMessagePush(WifiMessageQueue *queue);

QueueWifiMessagePushState QueueWifiMessagePop(WifiMessageQueue *queue);

uint32_t QueueWifiMessageNextIndex(WifiMessageQueue *queue, uint32_t index);

QueueWifiMessageStateEnum QueueWifiMessageState(WifiMessageQueue *queue);

QueueWifiMessageInit QueueWifiMessageBufferInit(WifiMessageQueue *queue, uint32_t size, uint32_t bufferSize);

#endif /* COMPONENTS_WIFICUSTOMCOMPONENT_QUEUEMESSAGEWIFI_H_ */
