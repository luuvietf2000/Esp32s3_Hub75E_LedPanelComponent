/*
 * GdmaConfig.h
 *
 *  Created on: Feb 7, 2026
 *      Author: viet.lv
 */

#ifndef COMPONENTS_LEDPANELCOMPONENT_GDMACONFIG_H_
#define COMPONENTS_LEDPANELCOMPONENT_GDMACONFIG_H_

#include "soc/gdma_reg.h"
#include <stdint.h>

#define GDMA_OUT_PERI_SEL_CHn_REG(x)					(GDMA_OUT_PERI_SEL_CH0_REG + 192 * x)
#define GDMA_CHANEL_LENGTH								5
#define PERIPHERAL_SIZE 								9
#define PERIPHERAL_RESET								0x3f

#define GDMA_OUT_CONF0_CHn_REG(x)						(GDMA_OUT_CONF0_CH0_REG + 192 * x)
#define GDMA_OUT_RST_CHn_IS_RESET						1
#define GDMA_OUT_RST_CHn_IS_UNRESET						0

#define DW0_OWNER_S										31
#define DW0_OWNER_V										1
#define DW0_OWNER_M										(DW0_OWNER_V << DW0_OWNER_S)
#define DW0_OWNER_GDMA									1

#define DW0_SUC_EOF_S									30
#define DW0_SUC_EOF_V									1
#define DW0_SUC_EOF_M									(DW0_SUC_EOF_V << DW0_SUC_EOF_S)
#define DWO_SUC_EOF_NO_ISR								0

#define DW0_SIZE_S										0
#define DW0_SIZE_V										0xfff
#define DW0_SIZE_M										(DW0_SIZE_V << DW0_SIZE_S) 

#define DW0_LENGTH_S									12
#define DW0_LENGTH_V									0xfff
#define DW0_LENGTH_M									(DW0_LENGTH_V << DW0_LENGTH_S)

#define DW0_ERR_EOF_S									28
#define DWO_ERR_EOF_V									1
#define DWO_ERR_EOF_M									(DWO_ERR_EOF_V << DW0_ERR_EOF_S)

#define DW_NULL											0
#define GDMA_OUT_LINK_CHn_REG(x)						(GDMA_OUT_LINK_CH0_REG + 192 * x)

#define GDMA_LCD_CAM									5
#define GDMA_OUTLINK_START_CHn_IS_START					1

#define SYSTEM_DMA_CLK_EN_IS_ENABLE						1
#define SYSTEM_DMA_RST_IS_RESET							1

#define GDMA_OUT_TOTAL_EOF_CH0_INT_CLR_IS_CLEAR			1
#define GDMA_OUT_INT_CLR_CHn_REG(x)						(GDMA_OUT_INT_CLR_CH0_REG + 192 *x)

typedef struct GdmaDescriptorsNode{
	uint32_t DW0;
	uint32_t DW1;
	uint32_t DW2;
} GdmaDescriptorsNode;

typedef struct GdmaConfig{
	uint32_t channel;
} GdmaConfig;

typedef enum{
	GDMA_CHANNEL_WORKING,
	GDMA_CHANNEL_IDLE
}GdmaChannelState;

typedef enum{
	SUC_EOF_DISABLE,
	SUC_EOF_ENABLE
} SucEofDw0;

typedef enum{
	GDMA_CHANNEL_FIND_AVAILABILITY_OK,
	GDMA_CHANNEL_FIND_AVAILABILITY_FAIL
} GdmaFindChannelState;

typedef enum{
	GDMA_INIT_OK,
	GDMA_INIT_FAIL_CAUSE_GDMA_CHANNEL_FIND_AVAILABILITY_FAIL
} GdmaInitState;

void GdmaClearIsrChannel(GdmaConfig *config);

GdmaChannelState GdmaChannelIsIdle(GdmaConfig *config);

void GdmaTransmit(GdmaConfig *config, uint32_t address);

GdmaInitState GdmaInit(GdmaConfig *config);

void SetDw0GdmaDescriptorsNode(GdmaDescriptorsNode *node, SucEofDw0 endFlagIsEnable,uint16_t length, uint16_t size);

void SetDw1GdmaDescriptorsNode(GdmaDescriptorsNode *node, uint32_t Dw1);

void SetDw2GdmaDescriptorsNode(GdmaDescriptorsNode *node, uint32_t Dw2);

void EnableGmdaTransmit(uint32_t channel);

GdmaFindChannelState SelectedGdmaChanel(uint32_t *channel);
void SetPeripheralConnectGdmaChanel(uint32_t channel);

void SetAddressFristGdmaTransmit(uint32_t channel, uint32_t address);

void ResetGdmaChanelAndFifoPointer(uint32_t channel);

void EnableAndResetGmdaClock();
#endif /* COMPONENTS_LEDPANELCOMPONENT_GDMACONFIG_H_ */
