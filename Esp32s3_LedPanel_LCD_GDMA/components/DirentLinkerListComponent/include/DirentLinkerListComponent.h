#ifndef DIRENT_LINKER_LIST_COMPONENT
#define DIRENT_LINKER_LIST_COMPONENT

#include <stdint.h>
typedef struct DirentNode{
	struct DirentNode * next;
	char *name;
} DirentNode;

typedef struct DirentLinkerList{
	struct DirentNode *head; 
	uint32_t size;
} DirentLinkerList;

void DirentLinkerListInit(DirentLinkerList *list);
void DirentLinkerListPush(DirentLinkerList *list, DirentNode *entry);
void DirentLinkerListDetelte(DirentLinkerList *list);
DirentNode* DirentLinkerListGetIndex(DirentLinkerList *list, uint32_t index);

#endif
