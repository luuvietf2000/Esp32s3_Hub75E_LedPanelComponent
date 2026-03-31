/*
 * QueueImage.h
 *
 *  Created on: Mar 1, 2026
 *      Author: viet.lv
 */

#ifndef COMPONENTS_LEDPANELCOMPONENT_INCLUDE_QUEUEIMAGERAW_H_
#define COMPONENTS_LEDPANELCOMPONENT_INCLUDE_QUEUEIMAGERAW_H_

#include "freertos/idf_additions.h"
#include "portmacro.h"
#include <stdint.h>

//-------------------------------------------------------------------------------------------------//

#define IMAGE_RAW_BUFF_OVERDUE_CELL							 										3
#define IMAGE_RAW_NO_BLOCK																			0
#define IMAGE_RAW_BLOCK																				portMAX_DELAY
//-------------------------------------------------------------------------------------------------//

typedef struct ImageRaw{
	uint8_t *buffer;
} ImageRaw;

typedef struct ImageRawConfig{
	uint32_t size;
	uint32_t width;
	uint32_t heigth;
} ImageRawConfig;

typedef struct QueueImageRaw{
	QueueHandle_t queueFree;
	QueueHandle_t queueReady;
	ImageRaw *head;
	ImageRawConfig config;
} QueueImageRaw;

//-------------------------------------------------------------------------------------------------//

typedef enum{
	QUEUE_IMAGE_RAW_INIT_OK,
	QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_QUEUE_FAIL,
	QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_IMAGE_RAW_USING_SPI_RAM_FAIL,
	QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_QUEUE_IMAGE_RAW_FAIL
} QueueImageInitEnum;

//-------------------------------------------------------------------------------------------------//

uint8_t GetSingelColorInPixel(ImageRawConfig *config, ImageRaw *raw, uint32_t colorSingel,uint32_t width, uint32_t heigth);

BaseType_t GetQueueImageRawReady(QueueImageRaw *queue, ImageRaw **pointer, TickType_t blocking);

BaseType_t PushQueueImageRawReady(QueueImageRaw *queue, ImageRaw *pointer, TickType_t blocking);

BaseType_t GetQueueImageRawFree(QueueImageRaw *queue, ImageRaw **pointer, TickType_t blocking);

BaseType_t PushQueueImageRawFree(QueueImageRaw *queue, ImageRaw *pointer, TickType_t blocking);

uint32_t GetColorImageRaw(ImageRawConfig *config, ImageRaw *raw, uint32_t width, uint32_t heigth);

void QueueImageRawFree(QueueImageRaw *queue, uint32_t lengthBufferAllocation);

QueueImageInitEnum QueueImageRawInit(QueueImageRaw *queue, int size, uint32_t width, uint32_t heigth);

#endif /* COMPONENTS_LEDPANELCOMPONENT_INCLUDE_QUEUEIMAGERAW_H_ */
