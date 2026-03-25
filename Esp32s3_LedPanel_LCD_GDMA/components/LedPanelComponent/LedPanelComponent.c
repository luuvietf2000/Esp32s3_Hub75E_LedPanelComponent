#include "LedPanelComponent.h"
#include "GdmaConfig.h"
#include "LcdCamConfig.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "soc/gpio_num.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "sys/unistd.h"

#define TAG_VECTOR_GDMA 												"Vector Gdma Descriptors Node Allocation"
#define TAG_VECTOR_DESCRIPTIONS_NODE									"Vector Gdma Descriptors Node"
#define TAG_CHECK_VECTOR_GMDA											"Vector Gdma Check is Valid"
#define VECTOR_GDMA_VALID_CONTENT										"Valid"
#define VECTOR_GDMA_INVALID_CONTENT										"Invalid"
#define VECTOR_GDMA_VALID_VALUE											1
#define VECTOR_GDMA_INVALID_VALUE										0
#define GDMA_DESCRIPTORS_NODE_VALUE										"Gdma Descriptors Node"
#define START_INTERNAL_DMA_RAM											0x3FC88000
#define END_INTERNAL_DMA_RAM											0x3FCFFFFF

#define START_EXTERNAL_DMA_RAM											0x3C000000
#define END_EXTERNAL_DMA_RAM											0x3DFFFFFF
#define QUEUE_VECTOR_DESCRIPTIORS_ERROR									"Queue Vector Gdma Descriptors Node Error"
#define QUEUE_VECTOR_DESCRIPTIORS_ERROR_CONTENT							"Queue Vector Gdma Descriptors Node Allow Error"
//--------------------------------------------------------------------------//
// Function test
void GdmaCheckVectorGdmaDescriptorsNode(VectorGdmaDescriptorsNode *vector){
	uint32_t isPass = VECTOR_GDMA_VALID_VALUE;
	ESP_LOGI(GDMA_DESCRIPTORS_NODE_VALUE, "Address Dw0 = 0x%08x, Dw0 = 0x%08x, Dw1 = 0x%08x, Dw2 = 0x%08x\n",
		(unsigned int) &vector->head[0].DW0,
		(unsigned int) vector->head[0].DW0,
		(unsigned int) vector->head[0].DW1,
		(unsigned int) vector->head[0].DW2
	);
	for(uint32_t i = 1; i < vector->length; i++){
		if(vector->head[i - 1].DW2 != (uint32_t) &vector->head[i].DW0)
			isPass = VECTOR_GDMA_INVALID_VALUE;
		
		if(START_INTERNAL_DMA_RAM > vector->head[i].DW1 		||
		 	vector->head[i].DW1 > END_INTERNAL_DMA_RAM  		
		 	/*||
			START_EXTERNAL_DMA_RAM > config->vector.head[i].DW1 	||
			config->vector.head[i].DW1 > END_EXTERNAL_DMA_RAM
			*/
			)
			isPass = VECTOR_GDMA_INVALID_VALUE;
		
		ESP_LOGI(GDMA_DESCRIPTORS_NODE_VALUE, "Address Dw0 = 0x%08x, Dw0 = 0x%08x, Dw1 = 0x%08x, Dw2 = 0x%08x\n", 
			(unsigned int) &vector->head[i].DW0,
			(unsigned int) vector->head[i].DW0,
			(unsigned int) vector->head[i].DW1,
			(unsigned int) vector->head[i].DW2
		);
	}
	ESP_LOGI(TAG_CHECK_VECTOR_GMDA, "Vector is %s\n", (isPass == VECTOR_GDMA_VALID_VALUE ? VECTOR_GDMA_VALID_CONTENT : VECTOR_GDMA_INVALID_CONTENT));
}

//--------------------------------------------------------------------------//

void LedPanelRemoveBuffer(){
	for(uint32_t i = 0; i < queueVectorGdmaDescriptorsNode->size; i++)
		VectorGdmaDescriptorsNodeClear(&(queueVectorGdmaDescriptorsNode + i)->vector);
	queueVectorGdmaDescriptorsNode->size = 0;
}

LedPanelTransmitState GetLedpanelState(LedPanelConfig *config){
	LcdTransmitState lcdState = GetLcdState();
	GdmaChannelState gmdaChannelState = GdmaChannelIsIdle(&config->gdmaConfig);
	if(lcdState == LCD_TRANSMIT_WORKING && gmdaChannelState == GDMA_CHANNEL_WORKING)
		return LEDPANEL_IDLE;
	return LEDPANEL_TRANSMIT_OK;
}

void LedPanelStop(LedPanelConfig *config){
	DisableGmdaTransmit(config->gdmaConfig.channel);
	LcdStop();
}

void LedPanelResetHW(LedPanelConfig *config){
	queueVectorGdmaDescriptorsNode->rear = queueVectorGdmaDescriptorsNode->front = -1;
	DisableGmdaTransmit(config->gdmaConfig.channel);
	ResetGdmaChanelAndFifoPointer(config->gdmaConfig.channel);
	LcdStop();
	ResetLcdCtrlAndTxFIFO();
}

QueueVectorGdmaDescriptorsNodePushState QueueVectorGdmaDescriptorsNodePush(LedPanelConfig *config, uint8_t *buffer){
	if(CheckQueueVectorGdmaDescriptorsNodeState() == QUEUE_VECTOR_DESCRIPTIORS_FULL)
		return QUEUE_VECTOR_DESCRIPTIORS_PUSH_FAIL_CAUSE_FULL;
	if(GetLedpanelState(config) == LEDPANEL_TRANSMIT_OK && queueVectorGdmaDescriptorsNode->size < 2)
		return QUEUE_VECTOR_DESCRIPTIORS_PUSH_FAIL_CAUSE_QUEUE_SIZE_ONE_AND_LEDPANEL_USING;
	
	int nextIndex = NextIndexQueueVectorGdmaDescriptorsNode(queueVectorGdmaDescriptorsNode->front);
	LedPanelConvertFrameData(&(queueVectorGdmaDescriptorsNode + nextIndex)->vector, &config->style, buffer);
	queueVectorGdmaDescriptorsNode->front = nextIndex;
	return QUEUE_VECTOR_DESCRIPTIORS_PUSH_OK;
}

LedPanelStartTransmitState RequestNextVectorGdmaDescriptorsNode(LedPanelConfig *config){
	QueueVectorGdmaDescriptorsNodeState queueState = CheckQueueVectorGdmaDescriptorsNodeState();
	if(queueVectorGdmaDescriptorsNode->size != 1 && queueState == QUEUE_VECTOR_DESCRIPTIORS_EMPTY)
		return LEDPANEL_START_TRANSMIT_FAIL_CAUSE_NO_GDMA_DESCRIPTORS_NODE_NEXT;
	int indexNext = NextIndexQueueVectorGdmaDescriptorsNode(queueVectorGdmaDescriptorsNode->rear);
	LedPanelStartTransmitState state = LedPanelStartTransmit(config, &((queueVectorGdmaDescriptorsNode + indexNext)->vector));
	queueVectorGdmaDescriptorsNode->rear = indexNext;
	return state;
}

QueueVectorGdmaDescriptorsNodeState CheckQueueVectorGdmaDescriptorsNodeState(){
	if(queueVectorGdmaDescriptorsNode->rear == queueVectorGdmaDescriptorsNode->front)
		return QUEUE_VECTOR_DESCRIPTIORS_EMPTY;
	else if(NextIndexQueueVectorGdmaDescriptorsNode(queueVectorGdmaDescriptorsNode->front) == queueVectorGdmaDescriptorsNode->rear)
		return QUEUE_VECTOR_DESCRIPTIORS_FULL;
	else 
		return QUEUE_VECTOR_DESCRIPTIORS_NORMAL;
}

int NextIndexQueueVectorGdmaDescriptorsNode(int index){
	return (index + 1) % queueVectorGdmaDescriptorsNode->size;
}

QueueVectorGdmaDescriptorsNodeInitState QueueVectorGdmaDescriptorsNodeInit(LedPanelStyle *style, uint32_t size){
	queueVectorGdmaDescriptorsNode = heap_caps_malloc( sizeof(QueueVectorGdmaDescriptorsNode) * size, MALLOC_CAP_8BIT);
	if(queueVectorGdmaDescriptorsNode == NULL){
		ESP_LOGE(QUEUE_VECTOR_DESCRIPTIORS_ERROR, QUEUE_VECTOR_DESCRIPTIORS_ERROR_CONTENT);
		return QUEUE_VECTOR_DESCRIPTIORS_INIT_ERROR_CAUSE_ALLOCATION_QUEUE_FAIL;
	}
	queueVectorGdmaDescriptorsNode->size = size;
	queueVectorGdmaDescriptorsNode->rear = queueVectorGdmaDescriptorsNode->front = 0;
	uint32_t bufferSize, vectorLength;
	LedPenalCaculatorVectorGmdaDescriptiorsLedPenal(style, &vectorLength, &bufferSize);
	for(uint32_t i = 0; i < size; i++){
		LedPanelVectorInitState vectorInitState =  VectorGdmaDescriptorsNodeInit(&(queueVectorGdmaDescriptorsNode + i)->vector,
																				 vectorLength,
																				 bufferSize, 
																				 HUB75E_INTERNAL_AREA
		);
		switch(vectorInitState){
			case LEDPANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL:
				return QUEUE_VECTOR_DESCRIPTIORS_INIT_ERROR_CAUSE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL;
			case LEDPANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL:
				return QUEUE_VECTOR_DESCRIPTIORS_INIT_ERROR_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL;
			case LEDPANEL_INIT_VECTOR_OK:
				break;
		}
	}
	
	return QUEUE_VECTOR_DESCRIPTIORS_INIT_OK;
}

LedPanelVectorInitState VectorGdmaDescriptorsNodeInit(VectorGdmaDescriptorsNode *vector, uint32_t length, uint32_t bufferSize, uint32_t areaMemory){	
	vector->head = heap_caps_malloc(sizeof(GdmaDescriptorsNode) * length, MALLOC_CAP_INTERNAL | MALLOC_CAP_32BIT);
	if(vector->head == NULL){
		ESP_LOGE( TAG_VECTOR_GDMA, "LEDPANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL\n");
		return LEDPANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL;
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
		ESP_LOGE( TAG_VECTOR_GDMA, "LEDPANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL\n");
		return LEDPANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL;
	}
	//-----------------------------------------------//
	for(uint32_t i = 0; i < length; i++){
		uint32_t isEndFrame = i != length - 1 ? SUC_EOF_DISABLE : SUC_EOF_ENABLE;
		SetDw0GdmaDescriptorsNode(&vector->head[i], isEndFrame, bufferSize,  bufferSize);
		SetDw1GdmaDescriptorsNode(&vector->head[i], (uint32_t) buffer + i * bufferSize);
	}
	
	return LEDPANEL_INIT_VECTOR_OK;
}

uint32_t GetColorStateLedPanelBit(uint32_t color, uint32_t formatColorBit, uint32_t placeColorBit, uint32_t startBit){
	return ((color >> (formatColorBit + placeColorBit)) & 1) << startBit;
}

uint32_t GetAddressLedPanelBit(uint32_t row, uint32_t mask, uint32_t start){
	return (row & mask) >> start;
}

uint32_t GetAddressLedPanel(uint32_t row){
	uint32_t a = GetAddressLedPanelBit(row, A_PIN_BIT_M, A_PIN_BIT_S) << A_PIN_S;
	uint32_t b = GetAddressLedPanelBit(row, B_PIN_BIT_M, B_PIN_BIT_S) << B_PIN_S;
	uint32_t c = GetAddressLedPanelBit(row, C_PIN_BIT_M, C_PIN_BIT_S) << C_PIN_S;
	uint32_t d = GetAddressLedPanelBit(row, D_PIN_BIT_M, D_PIN_BIT_S) << D_PIN_S;
	uint32_t e = GetAddressLedPanelBit(row, E_PIN_BIT_M, E_PIN_BIT_S) << E_PIN_S;
	return a | b | c | d | e;
}

uint32_t GetLedPanelColorPixel(uint32_t pixelUp, uint32_t pixelDown, uint32_t bit){
	uint32_t r1 = GetColorStateLedPanelBit(pixelUp, R_COLOR_S, bit, R1_PIN_S);
	uint32_t g1 = GetColorStateLedPanelBit(pixelUp, G_COLOR_S, bit, G1_PIN_S);
	uint32_t b1 = GetColorStateLedPanelBit(pixelUp, B_COLOR_S, bit, B1_PIN_S);
	
	uint32_t r2 = GetColorStateLedPanelBit(pixelDown, R_COLOR_S, bit, R2_PIN_S);
	uint32_t g2 = GetColorStateLedPanelBit(pixelDown, G_COLOR_S, bit, G2_PIN_S);
	uint32_t b2 = GetColorStateLedPanelBit(pixelDown, B_COLOR_S, bit, B2_PIN_S);
	
	return r1  | g1 | b1 | r2 | g2 | b2;
}

void LedPanelConvertFrameData(VectorGdmaDescriptorsNode *vector, LedPanelStyle *style, uint8_t *buffer){
	uint32_t pixelUpSide, pixelDownSide;
	uint32_t colorAfterConvert, addressAfterConvert;
	colorAfterConvert = addressAfterConvert = 0;
	uint16_t *bufferDestination = NULL;
	uint32_t frame = 0;
	uint32_t beforRowAddress;
	uint32_t bitColor = 0;
	uint32_t beforBitColor = 0;
	uint32_t width = style->width;
	uint32_t heigth = style->heigth;
	uint32_t color = style->color;
	uint32_t enablePixel = 0;
	for(uint16_t bit = 0; bit < color; bit++){
		for(uint16_t row = 0; row < heigth / 2; row++){
			bufferDestination = (uint16_t*)vector->head[frame].DW1;
			
			if(row == 0){
				beforBitColor = bit == 0 ? color - 1 : bit - 1;
				bitColor = beforBitColor;
				beforRowAddress = heigth / 2 - 1;
			} else {
				beforRowAddress = row - 1;
				bitColor = bit;
			}

			addressAfterConvert = GetAddressLedPanel(beforRowAddress);
			AddSignalOutputEnableLedPanel(bufferDestination, OE_CLOCK_CYCLES_START, addressAfterConvert);
			
			for(uint32_t phase = 0; phase < OE_CLOCK_CYCLES - OE_CLOCK_PHASE_1; phase++)
				AddSignalClkLedPanel(bufferDestination, OE_CLOCK_CYCLES_START + OE_CLOCK_PHASE_1 + phase, 0x00, addressAfterConvert, bitColor, enablePixel);
			
			for(uint16_t column = 0; column < width; column ++){	
				
				pixelUpSide =  GetColorImageRaw(buffer, column, row);
				pixelDownSide = GetColorImageRaw(buffer, column, row + heigth/ 2);
				
				pixelUpSide = Hub75ELutGetColor( pixelUpSide, color);
				pixelDownSide = Hub75ELutGetColor( pixelDownSide, color);
				
				colorAfterConvert = GetLedPanelColorPixel(pixelUpSide, pixelDownSide, bit);
			
				enablePixel = ((bit == 0) && (row == 0) && (column == 0)) ? 0 : 1;
				AddSignalClkLedPanel(bufferDestination, column + OE_CLOCK_CYCLES, colorAfterConvert, addressAfterConvert, bitColor, enablePixel); 
			}
			AddSignalLatchLedPanel(bufferDestination, OE_CLOCK_CYCLES + width - LATCH_CLOCK_CYCLES, colorAfterConvert, addressAfterConvert);
			frame++;
			
			bufferDestination = (uint16_t*)vector->head[frame].DW1;
			
			addressAfterConvert = GetAddressLedPanel(style->heigth / style->scan - 1);
			AddSignalOutputEnableLedPanel(bufferDestination, OE_CLOCK_CYCLES_START, addressAfterConvert);
			
			for(uint32_t phase = 0; phase < OE_CLOCK_CYCLES - OE_CLOCK_PHASE_1; phase++)
				AddSignalClkLedPanel(bufferDestination, OE_CLOCK_CYCLES_START + OE_CLOCK_PHASE_1 + phase, 0x00, addressAfterConvert, style->color - 1, enablePixel);
			
			for(uint32_t column = 0; column < (style->width + OE_CLOCK_CYCLES); column++)
				AddSignalClkLedPanel(bufferDestination, column + OE_CLOCK_CYCLES, 0x00, addressAfterConvert, style->color - 1, enablePixel); 
				
			AddSignalLatchLedPanel(bufferDestination, OE_CLOCK_CYCLES + width - LATCH_CLOCK_CYCLES, 0x00, addressAfterConvert);
		}
	}
}

void AddSignalClkLedPanel(uint16_t *buffer, uint32_t column, uint32_t color, uint32_t address, uint32_t bit, uint32_t enable){
	*(buffer + column) = color | address | (LATCH_DISABLE_BIT << LATCH_PIN_S) | (OE_BIT(enable, column, bit) << OUTPUT_ENABLE_PIN_S); 
}

void AddSignalLatchLedPanel(uint16_t *buffer, uint32_t index,  uint32_t color, uint32_t address){
	*(buffer + index) = address | color | (LATCH_ENABLE_BIT << LATCH_PIN_S) | (OE_DISABLE_BIT << OUTPUT_ENABLE_PIN_S);
}

void AddSignalOutputEnableLedPanel(uint16_t *buffer, uint32_t index, uint32_t address){
	*(buffer + index) = address | (LATCH_DISABLE_BIT << LATCH_PIN_S) | (OE_ENABLE_BIT << OUTPUT_ENABLE_PIN_S); 
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

LedPanelStartTransmitState LedPanelStartTransmit(LedPanelConfig *config, VectorGdmaDescriptorsNode *vector){
	GdmaChannelState gmdaChannelState = GdmaChannelIsIdle(&config->gdmaConfig);
	if(gmdaChannelState == GDMA_CHANNEL_WORKING){
		DisableGmdaTransmit(config->gdmaConfig.channel);
	}
		
	GdmaTransmit(&config->gdmaConfig, (uint32_t) &vector->head->DW0);
	
	LcdTransmitState lcdState = GetLcdState();
	if(lcdState == LCD_TRANSMIT_WORKING){
		LcdStop();
	}
		
	LcdStart();
	
	return LEDPANEL_START_TRANSMIT_OK;
}

void LedPenalCaculatorVectorGmdaDescriptiorsLedPenal(LedPanelStyle *style, uint32_t *length, uint32_t *size){
	*length = (style->heigth / style->scan * style->color) + LEDPANEL_DUMMY_PHASE;
	uint32_t bufferLength = style->width + OE_CLOCK_CYCLES;
	*size = bufferLength * sizeof(uint16_t);
}

LedPanelInitState LedPanelInit(LedPanelConfig *config, gpio_num_t pin[], uint32_t vectorGdmaDescriptiorsNodeSize, uint32_t queueImageRaw){
	QueueVectorGdmaDescriptorsNodeInitState queueInitState = QueueVectorGdmaDescriptorsNodeInit(&config->style, vectorGdmaDescriptiorsNodeSize);
	switch(queueInitState){
		case QUEUE_VECTOR_DESCRIPTIORS_INIT_ERROR_CAUSE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL:
			return LEDPANEL_INIT_FAIL_CAUSE_QUEUE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL;
		case QUEUE_VECTOR_DESCRIPTIORS_INIT_ERROR_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL:
			return LEDPANEL_INIT_FAIL_CAUSE_QUEUE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_BUFFER_FAIL;
		case QUEUE_VECTOR_DESCRIPTIORS_INIT_ERROR_CAUSE_ALLOCATION_QUEUE_FAIL:
			return LEDPANEL_INIT_FAIL_CAUSE_QUEUE_ALLOCATION_FAIL;
		case QUEUE_VECTOR_DESCRIPTIORS_INIT_OK:
			break;
	}
	QueueImageInitEnum queueRawInitState = QueueImageRawInit(queueImageRaw, config->style.width, config->style.heigth); 
	if(queueRawInitState != QUEUE_IMAGE_RAW_INIT_OK){
		return LEDPANEL_INIT_FAIL_CAUSE_QUEUE_INIT_FAIL;
	}
	Hub75ELutInit(config->style.gamma,
				  config->style.redScale,
				  config->style.greenScale,
				  config->style.blueScale
	);
	//-------------------------------------------------------------------------//
	
	LcdInit(pin);
	GdmaInitState gdmaInitstate =  GdmaInit(&config->gdmaConfig);
	if(gdmaInitstate == GDMA_INIT_FAIL_CAUSE_GDMA_CHANNEL_FIND_AVAILABILITY_FAIL)
		return LEDPANEL_INIT_FAIL_CAUSE_FIND_CHANNEL_AVAILABILITY_FAIL;
	
	return LEDPANEL_INIT_OK;
}