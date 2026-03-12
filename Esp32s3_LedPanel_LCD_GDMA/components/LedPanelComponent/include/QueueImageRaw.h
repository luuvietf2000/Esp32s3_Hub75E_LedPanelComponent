/*
 * QueueImage.h
 *
 *  Created on: Mar 1, 2026
 *      Author: viet.lv
 */

#ifndef COMPONENTS_LEDPANELCOMPONENT_INCLUDE_QUEUEIMAGERAW_H_
#define COMPONENTS_LEDPANELCOMPONENT_INCLUDE_QUEUEIMAGERAW_H_

#define IMAGE_RAW_BUFF_OVERDUE_CELL 										3


#include <stdint.h>
typedef struct ImageRaw{
	uint8_t *buffer;
} ImageRaw;

typedef struct QueueImageRaw{
	struct ImageRaw *head;
	uint32_t size;
	uint32_t rear;
	uint32_t front;
	uint32_t width;
	uint32_t heigth;
} QueueImageRaw;

typedef enum{
	QUEUE_IMAGE_RAW_FULL, 
	QUEUE_IMAGE_RAW_EMPTY,
	QUEUE_IMAGE_RAW_NOMAL
} QueueImageRawStateEnum;

typedef enum{
	QUEUE_IMAGE_RAW_PUSH_FAIL_CAUSE_QUEUE_FULL,
	QUEUE_IMAGE_RAW_PUSH_OK
} QueueImagePushStateEnum;

typedef enum{
	QUEUE_IMAGE_RAW_POP_FAIL_CAUSE_QUEUE_EMPTY,
	QUEUE_IMAGE_RAW_POP_OK
} QueueImagePopStateEnum;

typedef enum{
	QUEUE_IMAGE_RAW_INIT_OK,
	QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_QUEUE_FAIL,
	QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_IMAGE_RAW_USING_SPI_RAM_FAIL,
	QUEUE_IMAGE_RAW_INIT_FAIL_CAUSE_ALLOCATION_QUEUE_IMAGE_RAW_FAIL
} QueueImageInitEnum;

static QueueImageRaw *queueImageRaw;

QueueImagePopStateEnum PopQueueImageRaw();

QueueImagePushStateEnum PushQueuueImageRaw();

uint8_t* PeekHeadQueueImageRaw();

uint8_t* PeekTailQueueImageRaw();

QueueImageRawStateEnum GetQueueImageRawState();

int GetNextIndexQueueImageRaw(int index);

uint32_t GetColorImageRaw(uint8_t *buffer, uint32_t width, uint32_t heigth);

void QueueImageRawFree(uint32_t lengthBufferAllocation);

QueueImageInitEnum QueueImageRawInit(int size, uint32_t width, uint32_t heigth);

#endif /* COMPONENTS_LEDPANELCOMPONENT_INCLUDE_QUEUEIMAGERAW_H_ */
