#include "QueueMessageWifi.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"

#define TAG_UDP_CUSTOM															"Udp Custom"
#define QUEUE_WIFI_MESSAGE_FAIL_CAUSE_ALLOCATION_WIFI_MESSAGE_FAIL_CONTENT      "QUEUE_WIFI_MESSAGE_FAIL_CAUSE_ALLOCATION_WIFI_MESSAGE_FAIL"
#define QUEUE_WIFI_MESSAGE_FAIL_CAUSE_ALLOCATION_BUFFER_FAIL_CONTENT      		"QUEUE_WIFI_MESSAGE_FAIL_CAUSE_ALLOCATION_BUFFER_FAIL_CONTENT"
#define QUEUE_WIFI_MESSAGE_EMPTY_CONTENT										"QUEUE_WIFI_MESSAGE_EMPTY"
#define QUEUE_WIFI_MESSAGE_FULL_CONTENT											"QUEUE_WIFI_MESSAGE_FULL"
#define TAG_QUEUE_WIFI_MESSAGE													"Queue Wifi Messsage"

BaseType_t GetQueueWifiMessageMsgReady(QueueWifiMessage *queue, WifiMessage **msg, TickType_t blocking){
	BaseType_t result = xQueueReceive(queue->msgReady, msg, blocking);
	return result;
}

BaseType_t GetQueueWifiMessagePointerFree(QueueWifiMessage *queue, WifiMessage **msg, TickType_t blocking){
	BaseType_t result = xQueueReceive(queue->pointerFree, msg, blocking);
	return result;
}

BaseType_t PushQueueWifiMessageMsgReady(QueueWifiMessage *queue, WifiMessage *msg, TickType_t blocking){
	BaseType_t result = xQueueSend(queue->msgReady, &msg, blocking);
	return result;
}

BaseType_t PushQueueWifiMessagePointerFree(QueueWifiMessage *queue, WifiMessage *msg, TickType_t blocking){
	BaseType_t result = xQueueSend(queue->pointerFree, &msg, blocking);
	return result;
}

QueueWifiMessageInitState QueueWifiMessageInit(QueueWifiMessage *queue, uint32_t size, uint32_t bufferSize){
	queue->msgReady = xQueueCreate(size, sizeof(WifiMessage*));
	queue->pointerFree = xQueueCreate(size, sizeof(WifiMessage*));
	queue->head = heap_caps_malloc(sizeof(WifiMessage) * size, MALLOC_CAP_DEFAULT);
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
	for(uint32_t i = 0; i < size; i++){
		PushQueueWifiMessagePointerFree(queue, queue->head + i, QUEUE_MESSAGE_WIFI_BLOCK);
	}
	
	return QUEUE_WIFI_MESSAGE_INIT_OK;
}