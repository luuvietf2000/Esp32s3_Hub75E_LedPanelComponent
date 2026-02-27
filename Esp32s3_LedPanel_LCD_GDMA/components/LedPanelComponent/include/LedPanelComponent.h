#ifndef COMPONENTS_LEDPANELCOMPONENT_INCLUDE_LEDPANELCOMPONENT_H_
#define COMPONENTS_LEDPANELCOMPONENT_INCLUDE_LEDPANELCOMPONENT_H_

#include "soc/gpio_num.h"
#include "soc/gpio_num.h"

#define LEDPANEL_GDMA_DESCRIPTORS_NODE_END										1

#define LATCH_CLOCK_CYCLES														1
#define OE_CLOCK_CYCLES															2
#define OE_CLOCK_CYCLES_START													0
#define OE_CLOCK_PHASE_1														1
#define OE_CLOCK_PHASE_2														1

#define A_PIN_BIT_V																1
#define B_PIN_BIT_V																1
#define C_PIN_BIT_V																1
#define D_PIN_BIT_V																1
#define E_PIN_BIT_V																1

#define A_PIN_BIT_S																0
#define B_PIN_BIT_S																1
#define C_PIN_BIT_S																2
#define D_PIN_BIT_S																3
#define E_PIN_BIT_S																4

#define A_PIN_BIT_M																(A_PIN_BIT_V << A_PIN_BIT_S)
#define B_PIN_BIT_M																(B_PIN_BIT_V << B_PIN_BIT_S)
#define C_PIN_BIT_M																(C_PIN_BIT_V << C_PIN_BIT_S)
#define D_PIN_BIT_M																(D_PIN_BIT_V << D_PIN_BIT_S)
#define E_PIN_BIT_M																(E_PIN_BIT_V << E_PIN_BIT_S)

#define CLK_ENABLE_BIT															1
#define CLK_DISABLE_BIT															0

#define LATCH_ENABLE_BIT														1
#define LATCH_DISABLE_BIT														0

#define OE_ENABLE_BIT															0
#define OE_DISABLE_BIT															1
#define OE_BIT(column, place)													( (TIME_UINT(place) >= OE_CLOCK_CYCLES ) && (column < TIME_UINT(place)) ? OE_ENABLE_BIT : OE_DISABLE_BIT)

#define TIME_UINT(x)															(1 << x)

#define HUB75E_INTERNAL_AREA													MALLOC_CAP_INTERNAL
#define HUB75E_SPIRAM_AREA														MALLOC_CAP_SPIRAM
#include "GdmaConfig.h"
#include <stdint.h>

typedef enum{
	LEDPANEL_START_TRANSMIT_OK,
	LEDPANEL_START_TRANSMIT_FAIL_CAUSE_GDMA_CHANNEL_IS_WORKING,
	LEDPANEL_START_TRANSMIT_FAIL_CAUSE_LCD_TRANSMIT_IS_WORKING,
	LEDPANEL_START_TRANSMIT_FAIL_CAUSE_NO_GDMA_DESCRIPTORS_NODE_NEXT
}LedPanelStartTransmitState;

typedef enum{
	LEDPANEL_TRANSMIT_OK,
	LEDPANEL_TRANSMIT_FAIL_CAUSE_ALLOCATION_GDMA_DESCRIPTORS_NODE_FAIL,
	LEDPANEL_TRANSMIT_FAIL_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL,
	LEDPANEL_IDLE
}LedPanelTransmitState;

typedef enum{
	LEDPANEL_INIT_OK,
	LEDPANEL_INIT_FAIL_CAUSE_FIND_CHANNEL_AVAILABILITY_FAIL,
	LEDPANEL_INIT_FAIL_CAUSE_QUEUE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL,
	LEDPANEL_INIT_FAIL_CAUSE_QUEUE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_BUFFER_FAIL,
	LEDPANEL_INIT_FAIL_CAUSE_QUEUE_ALLOCATION_FAIL
}LedPanelInitState;

typedef enum{
	QUEUE_VECTOR_DESCRIPTIORS_INIT_OK,
	QUEUE_VECTOR_DESCRIPTIORS_INIT_ERROR_CAUSE_ALLOCATION_QUEUE_FAIL,
	QUEUE_VECTOR_DESCRIPTIORS_INIT_ERROR_CAUSE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL,
	QUEUE_VECTOR_DESCRIPTIORS_INIT_ERROR_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL
} QueueVectorGdmaDescriptorsNodeInitState;

typedef enum{
	ALLOCATION_OK,
	ALLOCATION_FAIL
}AllocationState;

typedef enum{
	LEDPANEL_SCAN_2_PART = 2
}LedPanelScan;

typedef enum{
	LEDPANEL_RGB666 = 6,
	LEDPANEL_RGB888 = 8
}LedPanelRgbColor;

typedef enum{
	LEDPANEL_INIT_VECTOR_OK,
	LEDPANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL,
	LEDPANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL
} LedPanelVectorInitState;

typedef enum{
	QUEUE_VECTOR_DESCRIPTIORS_EMPTY,
	QUEUE_VECTOR_DESCRIPTIORS_FULL,
	QUEUE_VECTOR_DESCRIPTIORS_NORMAL
}QueueVectorGdmaDescriptorsNodeState;

typedef enum{
	QUEUE_VECTOR_DESCRIPTIORS_PUSH_FAIL_CAUSE_FULL,
	QUEUE_VECTOR_DESCRIPTIORS_PUSH_FAIL_CAUSE_QUEUE_SIZE_ONE_AND_LEDPANEL_USING,
	QUEUE_VECTOR_DESCRIPTIORS_PUSH_OK
} QueueVectorGdmaDescriptorsNodePushState;

typedef struct VectorGdmaDescriptorsNode{
	GdmaDescriptorsNode *head;
	uint32_t length;
} VectorGdmaDescriptorsNode;

typedef struct LedPanelStyle{
	uint32_t width;
	uint32_t heigth;
	float redScale;
	float greenScale;
	float blueScale;
	float gamma;
	LedPanelScan scan;
	LedPanelRgbColor color;
} LedPanelStyle;
	
typedef struct LedPanelConfig{
	struct LedPanelStyle style;
	struct GdmaConfig gdmaConfig;
} LedPanelConfig;

typedef struct QueueVectorGdmaDescriptorsNode{
	VectorGdmaDescriptorsNode *head;
	int size;
	int rear;
	int front; 
}QueueVectorGdmaDescriptorsNode;

static struct QueueVectorGdmaDescriptorsNode *queueVectorGdmaDescriptorsNode;

//--------------------------------------------------------------------------//
// Function test
void GdmaCheckVectorGdmaDescriptorsNode(VectorGdmaDescriptorsNode *vector);
//--------------------------------------------------------------------------//

void LedPanelRemoveBuffer();

void LedPanelResetHW(LedPanelConfig *config);

LedPanelTransmitState GetLedpanelState(LedPanelConfig *config);

QueueVectorGdmaDescriptorsNodePushState QueueVectorGdmaDescriptorsNodePush(LedPanelConfig *config, uint32_t *buffer);

int NextIndexQueueVectorGdmaDescriptorsNode(int index);

LedPanelStartTransmitState RequestNextVectorGdmaDescriptorsNode(LedPanelConfig *config);

QueueVectorGdmaDescriptorsNodeState CheckQueueVectorGdmaDescriptorsNodeState();

QueueVectorGdmaDescriptorsNodeInitState QueueVectorGdmaDescriptorsNodeInit(LedPanelStyle *style, uint32_t size);

void LedPenalCaculatorVectorGmdaDescriptiorsLedPenal(LedPanelStyle *style, uint32_t *length, uint32_t *size);

void AddSignalOutputEnableLedPanel(uint16_t *buffer, uint32_t index, uint32_t address);

void AddSignalClkLedPanel(uint16_t *buffer, uint32_t column, uint32_t color, uint32_t address, uint32_t bit);

void AddSignalLatchLedPanel(uint16_t *buffer, uint32_t index,  uint32_t color, uint32_t address);

uint32_t GetLedPanelColorPixel(uint32_t pixelUp, uint32_t pixelDown, uint32_t bit);

uint32_t GetAddressLedPanel(uint32_t row);

LedPanelVectorInitState VectorGdmaDescriptorsNodeInit(VectorGdmaDescriptorsNode *vector, uint32_t length, uint32_t bufferSize, uint32_t areaMemory);

uint32_t GetColorStateLedPanelBit(uint32_t color, uint32_t formatColorBit, uint32_t placeColorBit, uint32_t startBit);

uint32_t GetAddressLedPanelBit(uint32_t row, uint32_t mask, uint32_t start);

void VectorGdmaDescriptorsNodeClear(VectorGdmaDescriptorsNode *vector);

uint32_t GetValueLedPanelColor(LedPanelRgbColor color);

uint32_t GetValueLedPanelScan(LedPanelScan scan);

void LedPanelConvertFrameData(VectorGdmaDescriptorsNode *vector, LedPanelStyle *style, uint32_t *buffer);

LedPanelStartTransmitState LedPanelStartTransmit(LedPanelConfig *config, VectorGdmaDescriptorsNode *vector);

LedPanelInitState LedPanelInit(LedPanelConfig *config, gpio_num_t pin[], uint32_t vectorGdmaDescriptiorsNodeSize);

#endif /* COMPONENTS_LEDPANELCOMPONENT_INCLUDE_LEDPANELCOMPONENT_H_ */