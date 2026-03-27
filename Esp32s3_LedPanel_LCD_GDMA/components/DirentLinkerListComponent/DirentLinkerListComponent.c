#include <stdio.h>
#include "DirentLinkerListComponent.h"

void DirentLinkerListInit(DirentLinkerList *list){
	list->size = 0;
}

void DirentLinkerListPush(DirentLinkerList *list, DirentNode *entry){
	list->size++;
	if(list->size == 1){
		list->head = entry;
		return;
	}
	DirentNode *current = DirentLinkerListGetIndex(list, list->size - 1 - 1);

	current->next = entry;
}

void DirentLinkerListDetelte(DirentLinkerList *list){
	if(list->size == 0)
		return;
	DirentNode *current = list->head;
	DirentNode *pre = NULL;
	while(current != NULL){
		pre = current;
		current = current->next;
		free(pre->name);
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