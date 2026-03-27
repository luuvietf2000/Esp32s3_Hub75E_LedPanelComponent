#include "QueueMessageWifi.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#define TAG_UDP_CUSTOM															"Udp Custom"
#define QUEUE_WIFI_MESSAGE_FAIL_CAUSE_ALLOCATION_WIFI_MESSAGE_FAIL_CONTENT      "QUEUE_WIFI_MESSAGE_FAIL_CAUSE_ALLOCATION_WIFI_MESSAGE_FAIL"
#define QUEUE_WIFI_MESSAGE_FAIL_CAUSE_ALLOCATION_BUFFER_FAIL_CONTENT      		"QUEUE_WIFI_MESSAGE_FAIL_CAUSE_ALLOCATION_BUFFER_FAIL_CONTENT"
#define QUEUE_WIFI_MESSAGE_EMPTY_CONTENT										"QUEUE_WIFI_MESSAGE_EMPTY"
#define QUEUE_WIFI_MESSAGE_FULL_CONTENT											"QUEUE_WIFI_MESSAGE_FULL"
#define TAG_QUEUE_WIFI_MESSAGE													"Queue Wifi Messsage"

WifiMessage *PeekHeadQueueWifiMessage(WifiMessageQueue *queue){
	return (queue->head + queue->rear);
}

WifiMessage *PeekTailQueueWifiMessage(WifiMessageQueue *queue){
	return (queue->head + queue->front);
}

QueueWifiMessagePopState QueueWifiMessagePush(WifiMessageQueue *queue){
	QueueWifiMessageStateEnum state = QueueWifiMessageState(queue);
	if(state == QUEUE_WIFI_MESSAGE_FULL)
		return QUEUE_WIFI_MESSAGE_PUSH_FAIL_CAUSE_QUEUE_FULL;
	uint32_t indexNext = QueueWifiMessageNextIndex(queue, queue->front);
	queue->front = indexNext;
	return QUEUE_WIFI_MESSAGE_PUSH_OK;
}

QueueWifiMessagePushState QueueWifiMessagePop(WifiMessageQueue *queue){
	QueueWifiMessageStateEnum state = QueueWifiMessageState(queue);
	if(state == QUEUE_WIFI_MESSAGE_EMPTY)
		return QUEUE_WIFI_MESSAGE_POP_FAIL_CAUSE_QUEUE_EMPTY;
	uint32_t indexNext = QueueWifiMessageNextIndex(queue, queue->rear);
	queue->rear = indexNext;
	return QUEUE_WIFI_MESSAGE_POP_OK;
}

uint32_t QueueWifiMessageNextIndex(WifiMessageQueue *queue, uint32_t index){
	uint32_t indexNext = (index + 1) % queue->size;
	return indexNext;
}

QueueWifiMessageStateEnum QueueWifiMessageState(WifiMessageQueue *queue){
	uint32_t nextIndex = QueueWifiMessageNextIndex(queue, queue->front);
	if(queue->rear == queue->front){
		//ESP_LOGI(TAG_QUEUE_WIFI_MESSAGE, QUEUE_WIFI_MESSAGE_EMPTY_CONTENT);
		return QUEUE_WIFI_MESSAGE_EMPTY;
	}
	else if(queue->rear == nextIndex){
		//ESP_LOGI(TAG_QUEUE_WIFI_MESSAGE, QUEUE_WIFI_MESSAGE_FULL_CONTENT);
		return QUEUE_WIFI_MESSAGE_FULL;
	}
	return QUEUE_WIFI_MESSAGE_NORMAL;
}

QueueWifiMessageInit QueueWifiMessageBufferInit(WifiMessageQueue *queue, uint32_t size, uint32_t bufferSize){
	queue->head = heap_caps_malloc(sizeof(WifiMessage) * size, MALLOC_CAP_DEFAULT);
	queue->size = size;
	queue->front = queue->rear = 0;
	if(queue->head == NULL){
		ESP_LOGE(TAG_UDP_CUSTOM, QUEUE_WIFI_MESSAGE_FAIL_CAUSE_ALLOCATION_WIFI_MESSAGE_FAIL_CONTENT);
		return QUEUE_WIFI_MESSAGE_FAIL_CAUSE_ALLOCATION_WIFI_MESSAGE_FAIL;
	}
	for(uint32_t i = 0; i < size; i++){
		(queue->head + i)->buffer = heap_caps_malloc(sizeof(char) * bufferSize, MALLOC_CAP_SPIRAM);
		if(((queue->head + i)->buffer) == NULL){
			ESP_LOGE(TAG_UDP_CUSTOM, QUEUE_WIFI_MESSAGE_FAIL_CAUSE_ALLOCATION_BUFFER_FAIL_CONTENT);
			return QUEUE_WIFI_MESSAGE_FAIL_CAUSE_ALLOCATION_BUFFER_FAIL;
		}
	}
	return QUEUE_WIFI_MESSAGE_INIT_OK;
}