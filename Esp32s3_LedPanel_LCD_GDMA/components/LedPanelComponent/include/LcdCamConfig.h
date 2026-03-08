#ifndef COMPONENTS_LEDPANELCOMPONENT_LCDCAMCONFIG_H_
#define COMPONENTS_LEDPANELCOMPONENT_LCDCAMCONFIG_H_

#include "soc/gpio_num.h"
#include "soc/gpio_sig_map.h"
#include <stdint.h>

// Set Frequency 20Mhz
#define LCD_CAM_CLK_EN_S_DEFFAULT						1
#define LCD_CAM_LCD_CLK_SEL_DEFFAULT	  				3

#define LCD_CAM_LCD_CLKM_DIV_NUM_DEFFAULT				16

#define LCD_CAM_LCD_CLKM_DIV_A_DEFFAULT					1
#define LCD_CAM_LCD_CLKM_DIV_B_DEFFAULT					0

#define M0												5

#define LCD_CAM_LCD_CLK_EQU_SYSCLK_DEFFAULT				1	
#define LCD_CAM_LCD_CLKCNT_N_DEFFAULT					(M0 - 1)


//Set Pin
#define PIN_SIZE 14
#define GPIO_FUNCx_OUT_SEL_CFG_REG_ADDRESS(x) 			(GPIO_FUNC0_OUT_SEL_CFG_REG + 0x4 * x) 
#define SIGNAL_NO_LCD_DATA_X(x)							(LCD_DATA_OUT0_IDX + x)	

#define GPIO_FUNCx_OEN_SEL_SIGNAL_PERIPHERAL 			0	
#define GPIO_FUNCx_OEN_INV_SEL_NOT_INVERT_SIGNAL		0	
#define GPIO_FUNCx_OUT_INV_SEL_NOT_INVERT_OUTPUT 		0

#define GPIO_ENABLE_REG_ADDRESS(x)						(x < 32 ? GPIO_ENABLE_W1TS_REG : GPIO_ENABLE1_W1TS_REG)						
#define GPIO_ENABLE_REG_BIT_S(x)						(x < 32 ? x : x - 32)
#define GPIO_ENABLE_REG_BIT_M(x)						(GPIO_ENABLE_REG_BIT_V << GPIO_ENABLE_REG_BIT_S(x))	
#define GPIO_ENABLE_REG_BIT_V							1
#define GPIO_ENABLE_REG_ENABLE							1

#define GPIO_PIN_REG(x)									(GPIO_PIN0_REG + 0x4 * x)
#define GPIO_PINx_PAD_DRIVER_NORMAL						0

#define IO_MUX_x_REG(x)									(IO_MUX_GPIO0_REG + 0x4 * x)
#define IO_MUX_MCU_SEL_FUNC1							0x1
#define IO_MUX_FUN_DRV_10MA								1
#define IO_MUX_FUN_WPD									1
#define IO_MUX_FUN_WPU									1

#define LCD_CAM_CAM_VH_DE_MODE_EN_DE_MODE				0
#define LCD_CAM_LCD_TRANS_DONE_INT_DISABLE				0

#define LCD_CAM_LCD_RGB_MODE_EN_DISABLE					0

#define LCD_CAM_LCD_CMD_DISABLE							0
#define LCD_CAM_LCD_CMD_ENABLE							1
#define LCD_CAM_LCD_DUMMY_DISABLE						0
#define LCD_CAM_LCD_DOUT_ENABLE							1

#define LCD_CAM_LCD_CMD_2_CYCLE_EN_1_CYCLE				0
#define LCD_CAM_LCD_ALWAYS_OUT_EN_CONTINUOS_MODE 		1
#define LCD_CAM_LCD_CD_DATA_SET_NOT_INVERT				0
#define LCD_CAM_LCD_CD_IDLE_EDGE_IS_HIGH				1
#define LCD_CAM_LCD_UPDATE_IS_UPDATE					1
#define LCD_CAM_LCD_RESET_IS_RESET						1
#define LCD_CAM_LCD_RESET_IS_UNRESET					1
#define LCD_CAM_LCD_AFIFO_RESET_IS_RESET				1
#define LCD_CAM_LCD_2BYTE_EN_16_BIT						1
#define LCD_CAM_LCD_BYTE_ORDER_NOT_INVERT				0
#define LCD_CAM_LCD_BYTE_ORDER_INVERT					0
#define LCD_CAM_LCD_BIT_ORDER_NOT_CHANGED				0
#define LCD_CAM_LCD_START_IS_START						1
#define LCD_CAM_LCD_START_IS_STOP						0
#define LCD_CAM_LCD_TRANS_DONE_INT_CLR_IS_CLEAR			1
#define LCD_CAM_LCD_DATA_DOUT_MODE_DELAY_FALLING		0x55555555
#define LCD_CAM_LCD_CK_OUT_EDGE_IS_FRIST_HALF_HIGH		0x1

typedef enum{
	LCD_TRANSMIT_IDLE,
	LCD_TRANSMIT_WORKING
}LcdTransmitState;

typedef enum{
	R1_PIN,
	G1_PIN,
	B1_PIN,
	R2_PIN,
	G2_PIN,
	B2_PIN,
	A_PIN,
	B_PIN,
	C_PIN, 
	D_PIN, 
	E_PIN, 
	LATCH_PIN, 
	OUTPUT_ENABLE_PIN,
	CLK_PIN
}IndexPinConfig;

typedef enum{
	R1_PIN_S,
	G1_PIN_S,
	B1_PIN_S,
	R2_PIN_S,
	G2_PIN_S,
	B2_PIN_S,
	A_PIN_S,
	B_PIN_S,
	C_PIN_S, 
	D_PIN_S, 
	E_PIN_S, 
	LATCH_PIN_S, 
	OUTPUT_ENABLE_PIN_S,
	CLK_PIN_S
}LedPanelPinBit;

void GpioConfig(uint32_t pin, uint32_t signal);
void LcdClearIsrFlag();
LcdTransmitState GetLcdState();
void LcdInit(gpio_num_t pin[]);
void LcdStop();
void LcdStart();
void UpdateLdcRegisters();
void ResetLcdCtrlAndTxFIFO();
void ConfigureCdSignalMode();
void ConfigureDoutPhase();
void ConfigureDummyPhase();
void ConfigureCmdPhase();
void DisableRgbMode();
void SetLcdInterrupts();
void MappingSignalAndGpio(gpio_num_t gpio_pin[]);
void SetLcdClock();

#endif /* COMPONENTS_LEDPANELCOMPONENT_LCDCAMCONFIG_H_ */
