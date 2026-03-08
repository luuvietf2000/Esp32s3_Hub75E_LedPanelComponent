/*
 * QueueImageRaw.c
 *
 *  Created on: Mar 1, 2026
 *      Author: viet.lv
 */


#include "QueueImageRaw.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <stdint.h>
#include <stdlib.h>
#include "Hub75ELut.h"

#define TAG_QUEUE_IMAGE_RAW_INIT 																	"QueueImageRawInit"
#define QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_QUEUE_FAIL_CONTENT								"QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_QUEUE_FAIL"
#define QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_IMAGE_RAW_USING_SPI_RAM_FAIL_CONTENT				"QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_IMAGE_RAW_USING_SPI_RAM_FAIL"
#define QUEUE_IMAGE_RAW_REQUEST_NEXT_FAIL_CAUSE_QUEUE_FULL_CONTENT									"QUEUE_IMAGE_RAW_REQUEST_NEXT_FAIL_CAUSE_QUEUE_FULL"
#define TAG_QUEUE_IMAGE_REQUEST_NEXT																"QueueImageRawRequestNext"
#define TAG_QUEUE_IMAGE_REQUEST_GET																	"QueueImageRawRequestGet"
#define QUEUE_IMAGE_RAW_REQUEST_GET_FAIL_CAUSE_QUEUE_EMPTY_CONTENT									"QUEUE_IMAGE_RAW_REQUEST_GET_FAIL_CAUSE_QUEUE_EMPTY"
#define QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_QUEUE_IMAGE_RAW_FAIL_CONTENT						"QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_QUEUE_IMAGE_RAW_FAIL"
#define QUEUE_IMAGE_RAW_POP_FAIL_CAUSE_QUEUE_EMPTY_CONTENT											"QUEUE_IMAGE_RAW_POP_FAIL_CAUSE_QUEUE_EMPTY"
QueueImagePopStateEnum PopQueueImageRaw(){
	QueueImageRawStateEnum state = GetQueueImageRawState();
	if(state != QUEUE_IMAGE_RAW_EMPTY){
		 uint32_t indeNext = GetNextIndexQueueImageRaw(queueImageRaw->rear);
		 queueImageRaw->rear = indeNext;
		 return QUEUE_IMAGE_RAW_POP_OK;
	}
	ESP_LOGI(TAG_QUEUE_IMAGE_REQUEST_NEXT, QUEUE_IMAGE_RAW_POP_FAIL_CAUSE_QUEUE_EMPTY_CONTENT);
	return QUEUE_IMAGE_RAW_POP_FAIL_CAUSE_QUEUE_EMPTY;
}

QueueImagePushStateEnum PushQueuueImageRaw(){
	QueueImageRawStateEnum state = GetQueueImageRawState();
	if(state != QUEUE_IMAGE_RAW_FULL){
		 uint32_t indeNext = GetNextIndexQueueImageRaw(queueImageRaw->front);
		 queueImageRaw->front = indeNext;
		 return QUEUE_IMAGE_RAW_PUSH_OK;
	}
	ESP_LOGI(TAG_QUEUE_IMAGE_REQUEST_NEXT, QUEUE_IMAGE_RAW_REQUEST_NEXT_FAIL_CAUSE_QUEUE_FULL_CONTENT);
	return QUEUE_IMAGE_RAW_PUSH_FAIL_CAUSE_QUEUE_FULL;
}

uint8_t* PeekHeadQueueImageRaw(){
	return (queueImageRaw->head + queueImageRaw->rear)->buffer;
}

uint8_t* PeekTailQueueImageRaw(){
	return (queueImageRaw->head + queueImageRaw->front)->buffer;
}

QueueImageRawStateEnum GetQueueImageRawState(){
	int indexNext = GetNextIndexQueueImageRaw(queueImageRaw->front);
	if(queueImageRaw->front == queueImageRaw->rear)
		return QUEUE_IMAGE_RAW_EMPTY;
	else if(indexNext == queueImageRaw->front)
		return QUEUE_IMAGE_RAW_FULL;
	return QUEUE_IMAGE_RAW_NOMAL;
}

int GetNextIndexQueueImageRaw(int index){
	int indexNext = (index + 1) % queueImageRaw->size;
	return indexNext;
}

uint32_t GetColorImageRaw(uint8_t *buffer, uint32_t width, uint32_t heigth){
	if(width >= queueImageRaw->width && heigth >= queueImageRaw->heigth)
		return 0;
	uint32_t index = (queueImageRaw->width * heigth + width) * HUB75E_LUT_COLOR;
	uint32_t color = (*(buffer + index + R_LUT_GAMMA_INDEX) << R_COLOR_S) | 
					 (*(buffer + index + G_LUT_GAMMA_INDEX) << G_COLOR_S) | 
					 (*(buffer + index + B_LUT_GAMMA_INDEX) << B_COLOR_S);
	return color;
}

void QueueImageRawFree(uint32_t lengthBufferAllocation){
	for(uint32_t i = 0; i < lengthBufferAllocation; i++){
		free((queueImageRaw->head + i)->buffer);
	}
	free(queueImageRaw);
}

QueueImageInitEnum QueueImageRawInit(int size, uint32_t width, uint32_t heigth){
	queueImageRaw = heap_caps_malloc(sizeof(QueueImageRaw), MALLOC_CAP_8BIT);
	if(queueImageRaw == NULL){
		ESP_LOGE(TAG_QUEUE_IMAGE_RAW_INIT, QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_QUEUE_IMAGE_RAW_FAIL_CONTENT);
		return QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_QUEUE_IMAGE_RAW_FAIL;
	}
	queueImageRaw->head = heap_caps_malloc(size * sizeof(ImageRaw), MALLOC_CAP_8BIT);
	if(queueImageRaw->head == NULL){
		ESP_LOGE(TAG_QUEUE_IMAGE_RAW_INIT, QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_QUEUE_FAIL_CONTENT);
		return QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_QUEUE_FAIL;
	}
	queueImageRaw->front = queueImageRaw->rear = -1;
	queueImageRaw->size = size;
	queueImageRaw->heigth = heigth;
	queueImageRaw->width = width;
	uint8_t *buffer;
	for(uint32_t i = 0; i < size; i++){
		buffer = heap_caps_malloc(sizeof(uint8_t) * width * heigth * IMAGE_RAW_BUFF_OVERDUE_CELL, MALLOC_CAP_SPIRAM);
		if(buffer == NULL){
			QueueImageRawFree(i);
			ESP_LOGE(TAG_QUEUE_IMAGE_RAW_INIT, QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_IMAGE_RAW_USING_SPI_RAM_FAIL_CONTENT);
			return IMAGE_RAW_BUFF_OVERDUE_CELL;
		}
		(queueImageRaw->head + i)->buffer = buffer;
	}
	return QUEUE_IMAGE_RAW_INIT_OK;
}