#include "LedPanelComponent.h"
#include "GdmaConfig.h"
#include "LcdCamConfig.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "soc/gpio_num.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "Hub75ELut.h"

#define TAG_VECTOR_GDMA 					"Vector Gdma Descriptors Node Allocation"
#define TAG_VECTOR_DESCRIPTIONS_NODE		"Vector Gdma Descriptors Node"
#define TAG_CHECK_VECTOR_GMDA				"Vector Gdma Check is Valid"
#define VECTOR_GDMA_VALID_CONTENT			"Valid"
#define VECTOR_GDMA_INVALID_CONTENT			"Invalid"
#define VECTOR_GDMA_VALID_VALUE				1
#define VECTOR_GDMA_INVALID_VALUE			0
#define GDMA_DESCRIPTORS_NODE_VALUE			"Gdma Descriptors Node"
#define START_INTERNAL_DMA_RAM				0x3FC88000
#define END_INTERNAL_DMA_RAM				0x3FCFFFFF

#define START_EXTERNAL_DMA_RAM				0x3C000000
#define END_EXTERNAL_DMA_RAM				0x3DFFFFFF

//--------------------------------------------------------------------------//
// Function test
void GdmaCheckVectorGdmaDescriptorsNode(LedPanelConfig *config){
	uint32_t isPass = VECTOR_GDMA_VALID_VALUE;
	ESP_LOGI(GDMA_DESCRIPTORS_NODE_VALUE, "Address Dw0 = 0x%08x, Dw0 = 0x%08x, Dw1 = 0x%08x, Dw2 = 0x%08x\n",
		(unsigned int) &config->vector.head[0].DW0,
		(unsigned int) config->vector.head[0].DW0,
		(unsigned int) config->vector.head[0].DW1,
		(unsigned int) config->vector.head[0].DW2
	);
	for(uint32_t i = 1; i < config->vector.length; i++){
		if(config->vector.head[i - 1].DW2 != (uint32_t) &config->vector.head[i].DW0)
			isPass = VECTOR_GDMA_INVALID_VALUE;
		
		if(START_INTERNAL_DMA_RAM > config->vector.head[i].DW1 		||
		 	config->vector.head[i].DW1 > END_INTERNAL_DMA_RAM  		
		 	/*||
			START_EXTERNAL_DMA_RAM > config->vector.head[i].DW1 	||
			config->vector.head[i].DW1 > END_EXTERNAL_DMA_RAM
			*/
			)
			isPass = VECTOR_GDMA_INVALID_VALUE;
		
		ESP_LOGI(GDMA_DESCRIPTORS_NODE_VALUE, "Address Dw0 = 0x%08x, Dw0 = 0x%08x, Dw1 = 0x%08x, Dw2 = 0x%08x\n", 
			(unsigned int) &config->vector.head[i].DW0,
			(unsigned int) config->vector.head[i].DW0,
			(unsigned int) config->vector.head[i].DW1,
			(unsigned int) config->vector.head[i].DW2
		);
	}
	ESP_LOGI(TAG_CHECK_VECTOR_GMDA, "Vector is %s\n", (isPass == VECTOR_GDMA_VALID_VALUE ? VECTOR_GDMA_VALID_CONTENT : VECTOR_GDMA_INVALID_CONTENT));
}

//--------------------------------------------------------------------------//

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
		/*buffer = heap_caps_malloc(bufferSize, MALLOC_CAP_DMA | areaMemory);
		//-----------------------------------------------//
		if(buffer == NULL){
			VectorGdmaDescriptorsNodeClear(vector);
			ESP_LOGE( TAG_VECTOR_GDMA, "LEDPANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL\n");
			return LEDPANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL;
		}
		*/
		//uint32_t isEndFrame = i != length - 1 ? SUC_EOF_DISABLE : SUC_EOF_ENABLE;
		SetDw0GdmaDescriptorsNode(&vector->head[i], SUC_EOF_DISABLE, bufferSize,  bufferSize);
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

void LedPanelConvertFrameData(VectorGdmaDescriptorsNode *vector, uint32_t width, uint32_t heigth, LedPanelRgbColor color, uint32_t *buffer){
	uint32_t pixelUpSide, pixelDownSide;
	uint32_t colorAfterConvert, addressAfterConvert;
	colorAfterConvert = addressAfterConvert = 0;
	uint16_t *bufferDestination = NULL;
	uint32_t frame = 0;
	uint32_t beforRowAddress;
	uint32_t bitColor = 0;
	uint32_t beforBitColor = 0;
	
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
				AddSignalClkLedPanel(bufferDestination, OE_CLOCK_CYCLES_START + OE_CLOCK_PHASE_1 + phase, 0x00, addressAfterConvert, bitColor);
			for(uint16_t column = 0; column < width; column ++){	
				
				pixelUpSide = *(buffer + row * width + column);
				pixelDownSide = *(buffer + (row + heigth / 2) * width + column);
				
				pixelUpSide = Hub75ELutGetColor( pixelUpSide, color);
				pixelDownSide = Hub75ELutGetColor( pixelDownSide, color);
				
				colorAfterConvert = GetLedPanelColorPixel(pixelUpSide, pixelDownSide, bit);
				
			
				AddSignalClkLedPanel(bufferDestination, column + OE_CLOCK_CYCLES, colorAfterConvert, addressAfterConvert, bitColor); 
			}
			AddSignalLatchLedPanel(bufferDestination, OE_CLOCK_CYCLES + width - LATCH_CLOCK_CYCLES, colorAfterConvert, addressAfterConvert);
			frame++;
		}
	}
}

void AddSignalClkLedPanel(uint16_t *buffer, uint32_t column, uint32_t color, uint32_t address, uint32_t bit){
	*(buffer + column) = color | address | (LATCH_DISABLE_BIT << LATCH_PIN_S) | (OE_BIT(column, bit) << OUTPUT_ENABLE_PIN_S); 
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
	if(buffer == NULL)
		return;
	heap_caps_free(vector->head);
	heap_caps_free(buffer);
	vector->length = 0;
}

LedPanelStartTransmitState LedPanelStartTransmit(LedPanelConfig *config){
	GdmaChannelState gmdaChannelState = GdmaChannelIsIdle(&config->gdmaConfig);
	if(gmdaChannelState == GDMA_CHANNEL_WORKING)
		return LEDPANEL_START_TRANSMIT_FAIL_CAUSE_GDMA_CHANNEL_IS_WORKING;
	GdmaTransmit(&config->gdmaConfig, (uint32_t) &config->vector.head->DW0);
	//GdmaClearIsrChannel(&config->gdmaConfig);
	
	LcdTransmitState lcdState = GetLcdState();
	if(lcdState == LCD_TRANSMIT_WORKING)
		return LEDPANEL_START_TRANSMIT_FAIL_CAUSE_LCD_TRANSMIT_IS_WORKING;	
	
	//LcdClearIsrFlag();
		
	LcdStart();
	
	return LEDPANEL_START_TRANSMIT_OK;
}

void LedPenalCaculatorVectorGmdaDescriptiorsLedPenal(LedPanelStyle *style, uint32_t *length, uint32_t *size){
	*length = (style->heigth / style->scan * style->color);
	uint32_t bufferLength = style->width + OE_CLOCK_CYCLES;
	*size = bufferLength * sizeof(uint16_t);
}

LedPanelInitState LedPanelInit(LedPanelConfig *config, gpio_num_t pin[]){
	uint32_t size, length;
	LedPenalCaculatorVectorGmdaDescriptiorsLedPenal(&config->style, &length, &size);
	//-------------------------------------------------------------------------//
	LedPanelVectorInitState vectorInitState =  VectorGdmaDescriptorsNodeInit(&config->vector,
																			 length,
																			 size, 
																			 MALLOC_CAP_INTERNAL
	);
	
	Hub75ELutInit(config->style.gamma,
				  config->style.redScale,
				  config->style.greenScale,
				  config->style.blueScale
	);
	//-------------------------------------------------------------------------//
	switch(vectorInitState){
		case LEDPANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL:
			return LEDPANEL_INIT_FAIL_CAUSE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_FAIL;
		case LEDPANEL_INIT_VECTOR_FAIL_CAUSE_ALLOCATION_BUFFER_ADDRESS_POINTER_FAIL:
			return LEDPANEL_INIT_FAIL_CAUSE_ALLOCATION_VECTOR_GDMA_DESCRIPTORS_NODE_BUFFER_FAIL;
		case LEDPANEL_INIT_VECTOR_OK:
			break;
	}
	LcdInit(pin);
	GdmaInitState state =  GdmaInit(&config->gdmaConfig);
	return state == GDMA_INIT_OK ? LEDPANEL_INIT_OK : LEDPANEL_INIT_FAIL_CAUSE_FAIL_FIND_CHANNEL_AVAILABILITY_FAIL;
}