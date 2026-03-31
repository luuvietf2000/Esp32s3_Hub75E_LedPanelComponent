#include "LedPanelComponent.h"
#include "GdmaConfig.h"
#include "Hub75ELut.h"
#include "LcdCamConfig.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
#include "soc/gpio_num.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "sys/unistd.h"

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
	DisableGmdaTransmit(config->gdmaConfig.channel);
	ResetGdmaChanelAndFifoPointer(config->gdmaConfig.channel); 
	LcdStop();
	ResetLcdCtrlAndTxFIFO();
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

void LedPanelConvertFrameData(VectorGdmaDescriptorsNode *vector, LedPanelStyle *style, ImageRawConfig *config, ImageRaw *raw){
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
	uint32_t r1, b1, g1, r2, g2, b2;
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
				
				//pixelUpSide =  GetColorImageRaw(config, raw, column, row);
				//pixelDownSide = GetColorImageRaw(config, raw, column, row + heigth/ 2);
				
				//pixelUpSide = Hub75ELutGetColor( pixelUpSide, color);
				//pixelDownSide = Hub75ELutGetColor( pixelDownSide, color);
				
				//colorAfterConvert = GetLedPanelColorPixel(pixelUpSide, pixelDownSide, bit);
			
				r1 = GetSingelColorInPixel(config, raw, R_LUT_GAMMA_INDEX, width, heigth);
				g1 = GetSingelColorInPixel(config, raw, G_LUT_GAMMA_INDEX, width, heigth);
				b1 = GetSingelColorInPixel(config, raw, B_LUT_GAMMA_INDEX, width, heigth);
				r2 = GetSingelColorInPixel(config, raw, R_LUT_GAMMA_INDEX, width, row + heigth/ 2);
				g2 = GetSingelColorInPixel(config, raw, G_LUT_GAMMA_INDEX, width, row + heigth/ 2);
				b2 = GetSingelColorInPixel(config, raw, B_LUT_GAMMA_INDEX, width, row + heigth/ 2);
				
				r1 = HUB75ELutGetSingelColor(R_LUT_GAMMA_INDEX, r1);
				g1 = HUB75ELutGetSingelColor(G_LUT_GAMMA_INDEX, g1);
				b1 = HUB75ELutGetSingelColor(B_LUT_GAMMA_INDEX, b1);
				r2 = HUB75ELutGetSingelColor(R_LUT_GAMMA_INDEX, r2);
				g2 = HUB75ELutGetSingelColor(G_LUT_GAMMA_INDEX, g2);
				b2 = HUB75ELutGetSingelColor(B_LUT_GAMMA_INDEX, b2);
				
				r1 = ((r1 & (1 << bit)) ? 1 : 0) << R1_PIN_S;
				g1 = ((g1 & (1 << bit)) ? 1 : 0) << G1_PIN_S;
				b1 = ((b1 & (1 << bit)) ? 1 : 0) << B1_PIN_S;

				r2 = ((r2 & (1 << bit)) ? 1 : 0) << R2_PIN_S;
				g2 = ((g2 & (1 << bit)) ? 1 : 0) << G2_PIN_S;
				b2 = ((b2 & (1 << bit)) ? 1 : 0) << B2_PIN_S;
				
				colorAfterConvert = r1 | g1 | b1 | r2 | g2 | b2;
				enablePixel = ((bit == 0) && (row == 0) && (column == 0)) ? 0 : 1;
				//AddSignalClkLedPanel(bufferDestination, column + OE_CLOCK_CYCLES, colorAfterConvert, addressAfterConvert, bitColor, enablePixel); 
				*(bufferDestination + column) = colorAfterConvert | addressAfterConvert | (LATCH_DISABLE_BIT << LATCH_PIN_S) | (OE_BIT(enablePixel, column + OE_CLOCK_CYCLES, bitColor) << OUTPUT_ENABLE_PIN_S); 
			}
			AddSignalLatchLedPanel(bufferDestination, OE_CLOCK_CYCLES + width - LATCH_CLOCK_CYCLES, colorAfterConvert, addressAfterConvert);
			frame++;
			
			bufferDestination = (uint16_t*)vector->head[frame].DW1;
			enablePixel = 1;
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

LedPanelInitState LedPanelInit(LedPanelConfig *config, gpio_num_t pin[], uint32_t vectorGdmaDescriptiorsNodeSize){
	uint32_t length, size;
	LedPenalCaculatorVectorGmdaDescriptiorsLedPenal(&config->style, &length, &size);
	BaseType_t queueInitState = DmaDescriptorManagerInit(vectorGdmaDescriptiorsNodeSize, length, size);
	if(queueInitState == pdFALSE)
		return LED_PANEL_INIT_FAIL_CAUSE_DMA_DESCRIPTORS_MANAGER_INIT_FAIL;
	Hub75ELutInit(config->style.color,
				  config->style.gamma,
				  config->style.redScale,
				  config->style.greenScale,
				  config->style.blueScale
	);
	//-------------------------------------------------------------------------//
	
	LcdInit(pin);
	GdmaInitState gdmaInitstate =  GdmaInit(&config->gdmaConfig);
	if(gdmaInitstate == GDMA_INIT_FAIL_CAUSE_GDMA_CHANNEL_FIND_AVAILABILITY_FAIL)
		return LED_PANEL_INIT_FAIL_CAUSE_FIND_CHANNEL_AVAILABILITY_FAIL;
	
	return LED_PANEL_INIT_OK;
}