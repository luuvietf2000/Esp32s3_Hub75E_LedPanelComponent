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
#include "freertos/idf_additions.h"
#include "portmacro.h"

#define TAG_QUEUE_IMAGE_RAW	 																	"QueueImageRawInit"


uint8_t GetSingelColorInPixel(ImageRawConfig *config, ImageRaw *raw, uint32_t colorSingel, uint32_t width, uint32_t heigth){
	uint32_t index = (config->width * heigth + width) * HUB75E_LUT_COLOR;
	return *(raw->buffer + index + colorSingel);
}

BaseType_t GetQueueImageRawReady(QueueImageRaw *queue, ImageRaw **pointer, TickType_t blocking){
	BaseType_t result = xQueueReceive(queue->queueReady, pointer, blocking);
	return result;
}

BaseType_t PushQueueImageRawReady(QueueImageRaw *queue, ImageRaw *pointer, TickType_t blocking){
	BaseType_t result = xQueueSend(queue->queueReady, &pointer, blocking);
	return result;
}

BaseType_t GetQueueImageRawFree(QueueImageRaw *queue, ImageRaw **pointer, TickType_t blocking){
	BaseType_t result = xQueueReceive(queue->queueFree, pointer, blocking);
	return result;
}

BaseType_t PushQueueImageRawFree(QueueImageRaw *queue, ImageRaw *pointer, TickType_t blocking){
	BaseType_t result = xQueueSend(queue->queueFree, &pointer, blocking);
	return result;
}

uint32_t GetColorImageRaw(ImageRawConfig *config, ImageRaw *raw, uint32_t width, uint32_t heigth){
	if(width >= config->width && heigth >= config->heigth)
		return 0;
	uint32_t index = (config->width * heigth + width) * HUB75E_LUT_COLOR;
	uint32_t color = (*(raw->buffer + index + R_LUT_GAMMA_INDEX) << R_COLOR_S) | 
					 (*(raw->buffer + index + G_LUT_GAMMA_INDEX) << G_COLOR_S) | 
					 (*(raw->buffer + index + B_LUT_GAMMA_INDEX) << B_COLOR_S);
	return color;
}

void QueueImageRawFree(QueueImageRaw *queue, uint32_t lengthBufferAllocation){
	for(uint32_t i = 0; i < lengthBufferAllocation; i++){
		free((queue->head + i)->buffer);
	}
	free(queue);
}

QueueImageInitEnum QueueImageRawInit(QueueImageRaw *queue, int size, uint32_t width, uint32_t heigth){
	queue->queueFree = xQueueCreate(size, sizeof(ImageRaw*));
	queue->queueReady = xQueueCreate(size, sizeof(ImageRaw*));
	queue->head = heap_caps_malloc(size * sizeof(ImageRaw), MALLOC_CAP_8BIT);
	if(queue->head == NULL){
		ESP_LOGE(TAG_QUEUE_IMAGE_RAW, "QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_QUEUE_FAIL_CONTENT");
		return QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_QUEUE_FAIL;
	}
	queue->config.size = size;
	queue->config.heigth = heigth;
	queue->config.width = width;
	uint8_t *buffer;
	for(uint32_t i = 0; i < size; i++){
		buffer = heap_caps_malloc(sizeof(uint8_t) * width * heigth * IMAGE_RAW_BUFF_OVERDUE_CELL, MALLOC_CAP_SPIRAM);
		if(buffer == NULL){
			QueueImageRawFree(queue, i);
			ESP_LOGE(TAG_QUEUE_IMAGE_RAW, "QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_IMAGE_RAW_USING_SPI_RAM_FAIL");
			return IMAGE_RAW_BUFF_OVERDUE_CELL;
		}
		(queue->head + i)->buffer = buffer;
	}
	for(uint32_t i = 0; i < size; i++)
		PushQueueImageRawFree(queue, queue->head + i, IMAGE_RAW_NO_BLOCK);
	return QUEUE_IMAGE_RAW_INIT_OK;
}