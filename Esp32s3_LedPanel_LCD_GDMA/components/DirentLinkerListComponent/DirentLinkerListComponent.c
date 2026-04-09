#include <stdint.h>
#include <stdio.h>
#include "DirentLinkerListComponent.h"
#include "esp_heap_caps.h"
#include "freertos/projdefs.h"
#include "portmacro.h"

BaseType_t DirentLinkerListInit(DirentLinkerList *list, uint32_t size, uint32_t bufferSize){

	for(uint32_t i = 0;  i < size; i++){
		DirentNode *newNode = heap_caps_malloc(sizeof(DirentNode), MALLOC_CAP_SPIRAM);
		newNode->buffer = heap_caps_malloc(255 * sizeof(char), MALLOC_CAP_SPIRAM);
		if(newNode->buffer == NULL){
			DirentLinkerListDetelte(list);
			return pdFALSE;
		}
		newNode->next = NULL;
		DirentLinkerListPush(list, newNode);
	}
	list->size = 0;
	list->width = size;
	return pdTRUE;
}

void DirentLinkerListPush(DirentLinkerList *list, DirentNode *entry){
	if(list->size == 0){
		list->head = entry;
	} else{
		DirentNode *current = DirentLinkerListGetIndex(list, list->size - 1);
		current->next = entry;
	}
	list->size++;
}

void DirentLinkerListDetelte(DirentLinkerList *list){
	if(list->size == 0)
		return;
	DirentNode *current = list->head;
	DirentNode *pre = NULL;
	while(current != NULL){
		pre = current;
		current = current->next;
		free(pre->buffer);
		free(pre);
	}
	list->head = NULL;
	list->size = 0;
}
DirentNode* DirentLinkerListGetIndex(DirentLinkerList *list, uint32_t index){
	if(index >= list->size)
		return NULL;
	DirentNode *current = list->head;
	uint32_t count = 0;
	while(count++ < index){
		current = current->next;
	}
	return current;
}