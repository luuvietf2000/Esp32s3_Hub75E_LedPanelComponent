/*
 * DmascriporsManager.h
 *
 *  Created on: Mar 29, 2026
 *      Author: viet.lv
 */

#ifndef COMPONENTS_LEDPANELCOMPONENT_DMASCRIPORSMANAGER_H_
#define COMPONENTS_LEDPANELCOMPONENT_DMASCRIPORSMANAGER_H_


#include "GdmaConfig.h"
#include "freertos/idf_additions.h"

//----------------------------------------------------------------------------------------------------------------//

#define HUB75E_INTERNAL_AREA													MALLOC_CAP_INTERNAL
#define HUB75E_SPIRAM_AREA														MALLOC_CAP_SPIRAM
#define DMA_MANAGER_TIME_NO_BLOCK												0
#define DMA_MANAGER_TIME_BLOCK													portMAX_DELAY

//----------------------------------------------------------------------------------------------------------------//
typedef enum{
	LED_PANEL_INIT_VECTOR_OK,
	LED_PANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL,
	LED_PANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL,
} LedPanelVectorInitStateEnum;

typedef enum{
	VECTOR_DESCRIPTIORS_POOL_INIT_OK,
	VECTOR_DESCRIPTIORS_POOL_INIT_ERROR_CAUSE_ALLOCATION_POOL_FAIL,
	VECTOR_DESCRIPTIORS_POOL_INIT_ERROR_CAUSE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL,
	VECTOR_DESCRIPTIORS_POOL_INIT_ERROR_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL
} VectorGdmaDescriptorsNodePoolInitStateEnum;
//----------------------------------------------------------------------------------------------------------------//

typedef struct VectorGdmaDescriptorsNode{
	GdmaDescriptorsNode *head;
	uint32_t length;
} VectorGdmaDescriptorsNode;

typedef struct QueueVectorGdmaDescriptorsNode{
	VectorGdmaDescriptorsNode *vector;
	uint32_t count;
} VectorGdmaDescriptorsNodePool;

//----------------------------------------------------------------------------------------------------------------//

//----------------------------------------------------------------------------------------------------------------//

VectorGdmaDescriptorsNode *GetDmaDescriptorTransmit();

void SetDmaDescriptorTransmit(VectorGdmaDescriptorsNode *vector);

BaseType_t DmaDescriptorManagerInit(uint32_t size, uint32_t vectorGdmaDescriptorsLength, uint32_t gdmaDescriptorBufferSize);

BaseType_t PushDmaDescriptorFree(VectorGdmaDescriptorsNode *pointer, TickType_t blocking);

BaseType_t GetDmaDescriptorFree(VectorGdmaDescriptorsNode **pointer, TickType_t blocking);

BaseType_t PushDmaDescriptorReady(VectorGdmaDescriptorsNode *pointer, TickType_t blocking);

BaseType_t GetDmaDescriptorReady(VectorGdmaDescriptorsNode **pointer, TickType_t blocking);

VectorGdmaDescriptorsNodePoolInitStateEnum VectorGdmaDescriptorsNodePoolInit(uint32_t sizePool, uint32_t vectorGdmaDescriptorsLength, uint32_t gdmaDescriptorBufferSize);

void VectorGdmaDescriptorsNodeClear(VectorGdmaDescriptorsNode *vector);

LedPanelVectorInitStateEnum VectorGdmaDescriptorsNodeInit(VectorGdmaDescriptorsNode *vector, uint32_t length, uint32_t bufferSize, uint32_t areaMemory);


#endif /* COMPONENTS_LEDPANELCOMPONENT_DMASCRIPORSMANAGER_H_ */
