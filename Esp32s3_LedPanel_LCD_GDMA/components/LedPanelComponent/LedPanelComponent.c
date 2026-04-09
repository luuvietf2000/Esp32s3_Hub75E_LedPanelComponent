#include "LedPanelComponent.h"
#include "DmascriporsManager.h"
#include "GdmaConfig.h"
#include "Hub75ELut.h"
#include "LcdCamConfig.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"
#include "soc/gpio_num.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "soc/interrupts.h"
#include "soc/soc.h"
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

static uint16_t *addressByteCode = NULL;
volatile DRAM_ATTR static BaseType_t isNotifyTask; 	
DRAM_ATTR static TaskHandle_t task = NULL;
static intr_handle_t ledPanelHandle;
DRAM_ATTR static uint32_t gdmaChannel;
DRAM_ATTR uint32_t addrDmaNew;

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

static inline void AddSignalOutputEnableLedPanel(uint16_t *buffer, uint32_t index, uint32_t address);

static inline void AddSignalClkLedPanel(uint16_t *buffer, uint32_t column, uint32_t color, uint32_t address, uint32_t bit);

static inline void AddSignalLatchLedPanel(uint16_t *buffer, uint32_t index,  uint32_t color, uint32_t address);

static void IRAM_ATTR ledPanelIsr(void *arg){
	uint32_t *channel = (uint32_t*) arg;
	uint32_t reg = REG_READ(GDMA_OUT_INT_ST_CHx_REG(*channel));
	if(reg & GDMA_OUT_EOF_CHn_INT_ST_M){
		REG_WRITE(GDMA_OUT_INT_CLR_CHn_REG(*channel), GDMA_OUT_EOF_CHn_INT_CLR);
		if(isNotifyTask == pdTRUE){
			uint32_t addr = REG_READ(GDMA_OUT_EOF_DES_ADDR_CHx_REG(*channel));
			if(addr == addrDmaNew){
				isNotifyTask = pdFALSE;
				BaseType_t xHigherPriorityTaskWoken = pdFALSE;
				vTaskNotifyGiveFromISR(task, &xHigherPriorityTaskWoken);
				portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
			}
		}
	}
}

BaseType_t LedPanelRestart(LedPanelConfig *config, VectorGdmaDescriptorsNode *vector){
	VectorGdmaDescriptorsNode *current = GetDmaDescriptorTransmit();
	if(current == NULL)
		return pdFALSE;
	GdmaDescriptorsNode *last = current->head + (current->length - 1);
	SetDw2GdmaDescriptorsNode(last, (uint32_t) &vector->head->DW0); 
	task = xTaskGetCurrentTaskHandle();
	addrDmaNew = (uint32_t) &vector->head->DW0;
	isNotifyTask = pdTRUE;
	GdmaSetRestartFunction(&config->gdmaConfig);
	while(isNotifyTask == pdTRUE){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	}
	SetDmaDescriptorTransmit(vector);
	SetDw2GdmaDescriptorsNode(last, (uint32_t) &current->head->DW0); 
	PushDmaDescriptorFree(current, DMA_MANAGER_TIME_BLOCK);
	return pdTRUE;
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
	uint32_t r1, b1, g1, r2, g2, b2;
	uint8_t *p1, *p2;
	uint32_t indexRowUp, indexRowDown;
	
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
			addressAfterConvert = *(addressByteCode + beforRowAddress);
			AddSignalOutputEnableLedPanel(bufferDestination, OE_CLOCK_CYCLES_START, addressAfterConvert);
			indexRowUp = row * width;
			indexRowDown = (row + 32) * width;
			for(uint32_t phase = 0; phase < OE_CLOCK_CYCLES - OE_CLOCK_PHASE_1; phase++)
				AddSignalClkLedPanel(bufferDestination, OE_CLOCK_CYCLES_START + OE_CLOCK_PHASE_1 + phase, 0x00, addressAfterConvert, bitColor);
			for(uint16_t column = 0; column < width; column ++){
				p1 = raw->buffer +  (indexRowUp + column) * HUB75E_LUT_COLOR;
				p2 = raw->buffer + (indexRowDown + column) * HUB75E_LUT_COLOR;
				
				r1 = lutGamma[R1_PIN_INDEX][p1[R_LUT_GAMMA_INDEX]][bit];
				g1 = lutGamma[G1_PIN_INDEX][p1[G_LUT_GAMMA_INDEX]][bit];
				b1 = lutGamma[B1_PIN_INDEX][p1[B_LUT_GAMMA_INDEX]][bit];
				
				r2 = lutGamma[R2_PIN_INDEX][p2[R_LUT_GAMMA_INDEX]][bit];
				g2 = lutGamma[G2_PIN_INDEX][p2[G_LUT_GAMMA_INDEX]][bit];
				b2 = lutGamma[B2_PIN_INDEX][p2[B_LUT_GAMMA_INDEX]][bit];
				
				colorAfterConvert = r1 | g1 | b1 | r2 | g2 | b2;
				AddSignalClkLedPanel(bufferDestination, column + OE_CLOCK_CYCLES, colorAfterConvert, addressAfterConvert, bitColor); 
			}
			AddSignalLatchLedPanel(bufferDestination, OE_CLOCK_CYCLES + width - LATCH_CLOCK_CYCLES, colorAfterConvert, addressAfterConvert);
				frame++;
		}
	}
}

static inline void AddSignalClkLedPanel(uint16_t *buffer, uint32_t column, uint32_t color, uint32_t address, uint32_t bit){
	*(buffer + column) = color | address | (LATCH_DISABLE_BIT << LATCH_PIN_S) | (OE_BIT(column, bit) << OUTPUT_ENABLE_PIN_S); 
}

static inline void AddSignalLatchLedPanel(uint16_t *buffer, uint32_t index,  uint32_t color, uint32_t address){
	*(buffer + index) = address | color | (LATCH_ENABLE_BIT << LATCH_PIN_S) | (OE_DISABLE_BIT << OUTPUT_ENABLE_PIN_S);
}

static inline void AddSignalOutputEnableLedPanel(uint16_t *buffer, uint32_t index, uint32_t address){
	*(buffer + index) = address | (LATCH_DISABLE_BIT << LATCH_PIN_S) | (OE_ENABLE_BIT << OUTPUT_ENABLE_PIN_S); 
}

LedPanelStartTransmitState LedPanelStartTransmit(LedPanelConfig *config, VectorGdmaDescriptorsNode *vector){
	DisableGmdaTransmit(config->gdmaConfig.channel);
	GdmaTransmit(&config->gdmaConfig, (uint32_t) &vector->head->DW0);
	LcdStop();
	LcdStart();
	SetDmaDescriptorTransmit(vector);
	return LEDPANEL_START_TRANSMIT_OK;
}

void LedPenalCaculatorVectorGmdaDescriptiorsLedPenal(LedPanelStyle *style, uint32_t *length, uint32_t *size){
	*length = (style->heigth / style->scan * style->color);
	uint32_t bufferLength = style->width + OE_CLOCK_CYCLES;
	*size = bufferLength * sizeof(uint16_t);
}

LedPanelInitState LedPanelInit(LedPanelConfig *config, gpio_num_t pin[], uint32_t vectorGdmaDescriptiorsNodeSize){
	uint32_t length, size;
	LedPenalCaculatorVectorGmdaDescriptiorsLedPenal(&config->style, &length, &size);
	BaseType_t queueInitState = DmaDescriptorManagerInit(vectorGdmaDescriptiorsNodeSize, length, size);
	if(queueInitState == pdFALSE)
		return LED_PANEL_INIT_FAIL_CAUSE_DMA_DESCRIPTORS_MANAGER_INIT_FAIL;
	Hub75ELutInit(R1_PIN_S, R2_PIN_S, G1_PIN_S, G2_PIN_S, B1_PIN_S, B2_PIN_S,
				  config->style.color,
				  config->style.gamma,
				  config->style.redScale,
				  config->style.greenScale,
				  config->style.blueScale
	);
	//-------------------------------------------------------------------------//
	GdmaInitState gdmaInitstate =  GdmaInit(&config->gdmaConfig);
	GdmaEnableIsrOutEof(&config->gdmaConfig);
	gdmaChannel = config->gdmaConfig.channel;
	esp_err_t  err = esp_intr_alloc(ETS_DMA_OUT_CHX_INTR_SOURCE(config->gdmaConfig.channel), ESP_INTR_FLAG_IRAM, ledPanelIsr, (void*) &gdmaChannel, &ledPanelHandle);
	//esp_err_t  err = esp_intr_alloc(ETS_DMA_OUT_CHX_INTR_SOURCE(config->gdmaConfig.channel), 0, ledPanelIsr, (void*) &gdmaChannel, &ledPanelHandle);
	LcdInit(pin);
	isNotifyTask = pdFALSE;
	
	if(gdmaInitstate == GDMA_INIT_FAIL_CAUSE_GDMA_CHANNEL_FIND_AVAILABILITY_FAIL)
		return LED_PANEL_INIT_FAIL_CAUSE_FIND_CHANNEL_AVAILABILITY_FAIL;
	
	uint32_t rowSide = config->style.heigth / config->style.scan;
	addressByteCode = heap_caps_malloc(sizeof(uint16_t) * rowSide, MALLOC_CAP_SPIRAM);
	for(uint32_t row = 0; row < rowSide; row++){
		*(addressByteCode + row) = GetAddressLedPanel(row);
	}
	
	return LED_PANEL_INIT_OK;
}