#ifndef COMPONENTS_WIFICUSTOMCOMPONENT_QUEUEMESSAGEWIFI_H_
#define COMPONENTS_WIFICUSTOMCOMPONENT_QUEUEMESSAGEWIFI_H_

#include "freertos/idf_additions.h"
#include "portmacro.h"
#include <stdint.h>

#define QUEUE_MESSAGE_WIFI_NO_BLOCK																		0
#define QUEUE_MESSAGE_WIFI_BLOCK																		portMAX_DELAY

typedef enum{
	QUEUE_WIFI_MESSAGE_INIT_OK,
	QUEUE_WIFI_MESSAGE_FAIL_CAUSE_ALLOCATION_WIFI_MESSAGE_FAIL,
	QUEUE_WIFI_MESSAGE_FAIL_CAUSE_ALLOCATION_BUFFER_FAIL
} QueueWifiMessageInitState;

typedef struct WifiMessage{
	int client;
	uint8_t *buffer;
	uint32_t length;
} WifiMessage;

typedef struct WifiMessageQueue{
	QueueHandle_t pointerFree;
	QueueHandle_t msgReady;
	WifiMessage *head;
} QueueWifiMessage;

BaseType_t PushQueueWifiMessagePointerFree(QueueWifiMessage *queue, WifiMessage *msg, TickType_t blocking);

BaseType_t GetQueueWifiMessageMsgReady(QueueWifiMessage *queue, WifiMessage **msg, TickType_t blocking);

BaseType_t GetQueueWifiMessagePointerFree(QueueWifiMessage *queue, WifiMessage **msg, TickType_t blocking);

BaseType_t PushQueueWifiMessageMsgReady(QueueWifiMessage *queue, WifiMessage *msg, TickType_t blocking);

QueueWifiMessageInitState QueueWifiMessageInit(QueueWifiMessage *queue, uint32_t size, uint32_t bufferSize);

#endif /* COMPONENTS_WIFICUSTOMCOMPONENT_QUEUEMESSAGEWIFI_H_ */
