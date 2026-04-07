#include "GdmaConfig.h"
#include <esp_heap_caps.h>
#include <stdint.h>
#include "RegCustom.h"
#include "esp_log.h"
#include "esp_private/gdma.h"
#include "esp_private/periph_ctrl.h"
#include "soc/gdma_reg.h"
#include "soc/soc.h"
#include "soc/system_reg.h"

#define TAG_SLECTION_GDMA_CHANEL 				"Slection Gdma Chanel"
#define TAG_GDMA_INIT							"Gdma Init"					

#define TAG_GDMA_CHECK_STATE_CHANNEL			"GDMA Check State Channel"
#define GDMA_IS_IDLE_CONTENT					"Idle"
#define GDMA_IS_WORKING_CONTENT					"Working"

void GdmaSetRestartFunction(GdmaConfig *config){
	WriteValueToAdress(GDMA_OUT_LINK_CHn_REG(config->channel), GDMA_OUTLINK_RESTART_CH0_S, GDMA_OUTLINK_RESTART_CH0_M, GDMA_OUTLINK_RESTART_CHx_Is_RESTART);
}

void GdmaEnableIsrOutEof(GdmaConfig *config){
	WriteValueToAdress(GDMA_OUT_INT_ENA_CHx_REG(config->channel), GDMA_OUT_EOF_CH0_INT_ENA_S, GDMA_OUT_EOF_CH0_INT_ENA_M, GDMA_OUT_EOF_CH0_INT_ENA_IS_ENABLE);
}

void GdmaClearIsrChannel(GdmaConfig *config){
	WriteValueToAdress(GDMA_OUT_INT_CLR_CHn_REG(config->channel), GDMA_OUT_TOTAL_EOF_CH0_INT_CLR_S,  GDMA_OUT_TOTAL_EOF_CH0_INT_CLR_M, GDMA_OUT_TOTAL_EOF_CH0_INT_CLR_IS_CLEAR);
}

GdmaChannelState GdmaChannelIsIdle(GdmaConfig *config){
	uint32_t reg = REG_READ(GDMA_OUT_LINK_CHn_REG(config->channel));
	uint32_t state = GetRegValue(reg, GDMA_OUTLINK_PARK_CH0_M,  GDMA_OUTLINK_PARK_CH0_S);
	ESP_LOGI(TAG_GDMA_CHECK_STATE_CHANNEL, "Channel Is %s\n", state == GDMA_CHANNEL_IDLE ? GDMA_IS_IDLE_CONTENT : GDMA_IS_WORKING_CONTENT);
	return state;
}

void GdmaTransmit(GdmaConfig *config, uint32_t address){
	SetAddressFristGdmaTransmit(config->channel, address);
	EnableGmdaTransmit(config->channel);
}

GdmaInitState GdmaInit(GdmaConfig *config){
	periph_module_enable(PERIPH_GDMA_MODULE);
	//EnableAndResetGmdaClock();
	GdmaFindChannelState state = SelectedGdmaChanel(&config->channel);
	if(state == GDMA_CHANNEL_FIND_AVAILABILITY_FAIL){
		ESP_LOGE(TAG_GDMA_INIT, "GDMA configure fail\n");
		return GDMA_INIT_FAIL_CAUSE_GDMA_CHANNEL_FIND_AVAILABILITY_FAIL;
	}
	ResetGdmaChanelAndFifoPointer(config->channel);
	SetPeripheralConnectGdmaChanel(config->channel);
	ESP_LOGI(TAG_GDMA_INIT, "GDMA configure ok\n");
	return GDMA_INIT_OK;
}

void DisableGmdaTransmit(uint32_t channel){
	WriteValueToAdress(GDMA_OUT_LINK_CHn_REG(channel), GDMA_OUTLINK_STOP_CH0_S, GDMA_OUTLINK_STOP_CH0_M, GDMA_OUTLINK_START_CHn_IS_STOP);
}

void EnableGmdaTransmit(uint32_t channel){
	//  Enable GDMA’s transmit channel for data transfer
	WriteValueToAdress(GDMA_OUT_LINK_CHn_REG(channel), GDMA_OUTLINK_START_CH0_S, GDMA_OUTLINK_START_CH0_M, GDMA_OUTLINK_START_CHn_IS_START);
}

void SetPeripheralConnectGdmaChanel(uint32_t channel){
	// Set peripheral to be connected
	WriteValueToAdress(GDMA_OUT_PERI_SEL_CHn_REG(channel), GDMA_PERI_OUT_SEL_CH0_S, GDMA_PERI_OUT_SEL_CH0_M, GDMA_LCD_CAM);
}

void SetAddressFristGdmaTransmit(uint32_t channel, uint32_t address){
	// Set address of the first transmit descriptor
	WriteValueToAdress(GDMA_OUT_LINK_CHn_REG(channel), GDMA_OUTLINK_ADDR_CH0_S, GDMA_OUTLINK_ADDR_CH0_M, address);
}

void SetDw0GdmaDescriptorsNode(GdmaDescriptorsNode *node, SucEofDw0 endFlagIsEnable,uint16_t length, uint16_t size){
	// Modify Dw0 GdmaDescriptorsNode 
	node->DW0 = ((DW0_OWNER_GDMA << DW0_OWNER_S) & DW0_OWNER_M)| ((endFlagIsEnable << DW0_SUC_EOF_S) & DW0_SUC_EOF_M) | (DW0_SIZE_M & (size << DW0_SIZE_S)) | (DW0_LENGTH_M & (length << DW0_LENGTH_S));
}

void SetOwnerDw0DescriptorsNode(GdmaDescriptorsNode *node, uint32_t owner){
	uint32_t reg = node->DW0 & (~DW0_OWNER_M);
	node->DW0 = reg & (owner << DW0_OWNER_S);
}

void SetDw1GdmaDescriptorsNode(GdmaDescriptorsNode *node, uint32_t Dw1){
	// Modify Dw1 GdmaDescriptorsNode
	node->DW1 = Dw1;
}

void SetDw2GdmaDescriptorsNode(GdmaDescriptorsNode *node, uint32_t Dw2){
	// Modify Dw2 GdmaDescriptorsNode
	node->DW2 = Dw2;
}

GdmaFindChannelState SelectedGdmaChanel(uint32_t *channel){
	// Find channel availability 
	/*
	for(uint8_t i = 0; i < GDMA_CHANEL_LENGTH; i++){
		uint32_t reg = REG_READ(GDMA_OUT_PERI_SEL_CHn_REG(i));
		uint8_t peripheral = reg & GDMA_PERI_OUT_SEL_CH0_M;
		if(peripheral == PERIPHERAL_RESET){
			*channel = i;
			ESP_LOGI(TAG_SLECTION_GDMA_CHANEL, "Selected channel %ld \n", (unsigned long) *channel);
			return GDMA_CHANNEL_FIND_AVAILABILITY_OK; 
		}
	}
	*/
	
	gdma_channel_handle_t dmaChan;

	gdma_channel_alloc_config_t dmaCfg = {
	    .direction = GDMA_CHANNEL_DIRECTION_TX,
	};
	
	ESP_ERROR_CHECK(gdma_new_ahb_channel(&dmaCfg, &dmaChan));

	int peripheral = PERIPHERAL_RESET;
	ESP_ERROR_CHECK(gdma_get_channel_id(dmaChan, &peripheral));
	if(peripheral != PERIPHERAL_RESET){
		*channel = peripheral;
		ESP_LOGI(TAG_SLECTION_GDMA_CHANEL, "Selected channel %ld \n", (unsigned long) *channel);
		return GDMA_CHANNEL_FIND_AVAILABILITY_OK; 
	}
	ESP_LOGE(TAG_SLECTION_GDMA_CHANEL, "No channel availability\n");
	return GDMA_INIT_FAIL_CAUSE_GDMA_CHANNEL_FIND_AVAILABILITY_FAIL;
}

void ResetGdmaChanelAndFifoPointer(uint32_t channel){
	// Reset the state machine of GDMA’s transmit channel and FIFO pointer
	WriteValueToAdress(GDMA_OUT_CONF0_CHn_REG(channel), GDMA_OUT_RST_CH0_S, GDMA_OUT_RST_CH0_M, GDMA_OUT_RST_CHn_IS_RESET);
	WriteValueToAdress(GDMA_OUT_CONF0_CHn_REG(channel), GDMA_OUT_RST_CH0_S, GDMA_OUT_RST_CH0_M, GDMA_OUT_RST_CHn_IS_UNRESET);
}

void EnableAndResetGmdaClock(){
	// GDMA’s clock and reset
	WriteValueToAdress(SYSTEM_PERIP_CLK_EN1_REG, SYSTEM_DMA_CLK_EN_S, SYSTEM_DMA_CLK_EN_M, SYSTEM_DMA_CLK_EN_IS_ENABLE);
	WriteValueToAdress(SYSTEM_PERIP_RST_EN1_REG, SYSTEM_DMA_RST_S, SYSTEM_DMA_RST_M, SYSTEM_DMA_RST_IS_RESET);
}