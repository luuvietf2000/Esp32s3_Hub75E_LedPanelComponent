#ifndef DIRENT_LINKER_LIST_COMPONENT
#define DIRENT_LINKER_LIST_COMPONENT

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>
typedef struct DirentNode{
	struct DirentNode * next;
	uint8_t *buffer;
} DirentNode;

typedef struct DirentLinkerList{
	struct DirentNode *head; 
	uint32_t size;
	uint32_t width;
} DirentLinkerList;

BaseType_t DirentLinkerListInit(DirentLinkerList *list, uint32_t size, uint32_t bufferSize);
void DirentLinkerListPush(DirentLinkerList *list, DirentNode *entry);
void DirentLinkerListDetelte(DirentLinkerList *list);
DirentNode* DirentLinkerListGetIndex(DirentLinkerList *list, uint32_t index);

#endif
