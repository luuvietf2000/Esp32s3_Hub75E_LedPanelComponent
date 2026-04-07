#include "DmascriporsManager.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"
#include <stdint.h>

#define TAG_VECTOR_GDMA 												"Dma Descriptor manager"

static VectorGdmaDescriptorsNodePool pool;
static QueueHandle_t vectorFree = NULL;
static QueueHandle_t vectorReady = NULL;
static VectorGdmaDescriptorsNode *vectorTransmit = NULL;

VectorGdmaDescriptorsNode *GetDmaDescriptorTransmit(){
	return vectorTransmit;
}

void SetDmaDescriptorTransmit(VectorGdmaDescriptorsNode *vector){
	vectorTransmit = vector;
}

BaseType_t DmaDescriptorManagerInit(uint32_t size, uint32_t vectorGdmaDescriptorsLength, uint32_t gdmaDescriptorBufferSize){
	vectorFree = xQueueCreate(size, sizeof(VectorGdmaDescriptorsNode*));
	vectorReady = xQueueCreate(size, sizeof(VectorGdmaDescriptorsNode*));
	if(VectorGdmaDescriptorsNodePoolInit(size, vectorGdmaDescriptorsLength, gdmaDescriptorBufferSize) != VECTOR_DESCRIPTIORS_POOL_INIT_OK)
		return pdFALSE;
	for(uint32_t i = 0; i < size; i++)
		PushDmaDescriptorFree(pool.vector + i, DMA_MANAGER_TIME_NO_BLOCK);
	return pdTRUE;
}

BaseType_t PushDmaDescriptorFree(VectorGdmaDescriptorsNode *pointer, TickType_t blocking){
	BaseType_t result = xQueueSend(vectorFree, &pointer, blocking);
	return result;
}

BaseType_t GetDmaDescriptorFree(VectorGdmaDescriptorsNode **pointer, TickType_t blocking){
	BaseType_t result = xQueueReceive(vectorFree, pointer, blocking);
	return result;
}

BaseType_t PushDmaDescriptorReady(VectorGdmaDescriptorsNode *pointer, TickType_t blocking){
	BaseType_t result = xQueueSend(vectorReady, &pointer, blocking);
	return result;
}

BaseType_t GetDmaDescriptorReady(VectorGdmaDescriptorsNode **pointer, TickType_t blocking){
	BaseType_t result = xQueueReceive(vectorReady, pointer, blocking);
	return result;
}

VectorGdmaDescriptorsNodePoolInitStateEnum VectorGdmaDescriptorsNodePoolInit(uint32_t sizePool, uint32_t vectorGdmaDescriptorsLength, uint32_t gdmaDescriptorBufferSize){
	pool.vector = heap_caps_malloc( sizeof(VectorGdmaDescriptorsNode) * sizePool, MALLOC_CAP_8BIT);
	if(pool.vector == NULL){
		ESP_LOGE(TAG_VECTOR_GDMA, "VECTOR_DESCRIPTIORS_POOL_INIT_ERROR_CAUSE_ALLOCATION_POOL_FAIL\n");
		return VECTOR_DESCRIPTIORS_POOL_INIT_ERROR_CAUSE_ALLOCATION_POOL_FAIL;
	}
	pool.count = sizePool;
	for(uint32_t i = 0; i < sizePool; i++){
		LedPanelVectorInitStateEnum vectorInitState =  VectorGdmaDescriptorsNodeInit(pool.vector + i,
																				 	vectorGdmaDescriptorsLength,
																				 gdmaDescriptorBufferSize, 
																				 HUB75E_INTERNAL_AREA
		);
		switch(vectorInitState){
			case LED_PANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL:
				return VECTOR_DESCRIPTIORS_POOL_INIT_ERROR_CAUSE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL;
			case LED_PANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL:
				return VECTOR_DESCRIPTIORS_POOL_INIT_ERROR_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL;
			case LED_PANEL_INIT_VECTOR_OK:
				break;
		}
	}
	
	return VECTOR_DESCRIPTIORS_POOL_INIT_OK;
}

void VectorGdmaDescriptorsNodeClear(VectorGdmaDescriptorsNode *vector){
	if(vector->head == NULL)
		return;
	uint32_t *buffer = (uint32_t*) vector->head[0].DW1;
	heap_caps_free(vector->head);
	if(buffer == NULL)
		return;
	heap_caps_free(buffer);
	vector->length = 0;
}

LedPanelVectorInitStateEnum VectorGdmaDescriptorsNodeInit(VectorGdmaDescriptorsNode *vector, uint32_t length, uint32_t bufferSize, uint32_t areaMemory){	
	vector->head = heap_caps_malloc(sizeof(GdmaDescriptorsNode) * length, MALLOC_CAP_INTERNAL | MALLOC_CAP_32BIT);
	if(vector->head == NULL){
		ESP_LOGE( TAG_VECTOR_GDMA, "LED_PANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL\n");
		return LED_PANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL;
	}
	
	for(uint32_t index = 0; index < length - 1; index++){
		SetDw2GdmaDescriptorsNode(&vector->head[index], (uint32_t) &vector->head[index + 1].DW0);
	}
	SetDw2GdmaDescriptorsNode(&vector->head[length - 1], (uint32_t) &vector->head[0].DW0);
	//SetDw2GdmaDescriptorsNode(&vector->head[length - 1], 0);
	vector->length = length;
	
	uint32_t *buffer = NULL;
	buffer = heap_caps_malloc(bufferSize * length, MALLOC_CAP_DMA | areaMemory);
	if(buffer == NULL){
		VectorGdmaDescriptorsNodeClear(vector);
		ESP_LOGE( TAG_VECTOR_GDMA, "LED_PANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL\n");
		return LED_PANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL;
	}
	//-----------------------------------------------//
	for(uint32_t i = 0; i < length; i++){
		uint32_t isEndFrame = i == 0 ? SUC_EOF_ENABLE : SUC_EOF_DISABLE;
		SetDw0GdmaDescriptorsNode(vector->head + i, isEndFrame, bufferSize,  bufferSize);
		SetDw1GdmaDescriptorsNode(vector->head + i, (uint32_t) buffer + i * bufferSize);
	}
	
	return LED_PANEL_INIT_VECTOR_OK;
}