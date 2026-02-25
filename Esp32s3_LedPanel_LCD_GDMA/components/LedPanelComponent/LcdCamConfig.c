#include "LcdCamConfig.h"
#include "RegCustom.h"
#include <soc/lcd_cam_reg.h>
#include <soc/gpio_reg.h>
#include <stdint.h>
#include "esp_log.h"
#include "esp_private/periph_ctrl.h"
#include "soc/gpio_num.h"
#include "soc/io_mux_reg.h"
#include "soc/periph_defs.h"

#define TAG_LCD_STATE 								"Lcd state"
#define TAG_LCD_IDLE 								"Lcd idle"
#define TAG_LCD_WORKING 							"Lcd working"


void LcdClearIsrFlag(){
	WriteValueToAdress(LCD_CAM_LC_DMA_INT_CLR_REG, LCD_CAM_LCD_TRANS_DONE_INT_CLR_S, LCD_CAM_LCD_TRANS_DONE_INT_CLR_M, LCD_CAM_LCD_TRANS_DONE_INT_CLR_IS_CLEAR);
}

LcdTransmitState GetLcdState(){
	LcdTransmitState state = GetRegValue(LCD_CAM_LCD_USER_REG, LCD_CAM_LCD_START_M, LCD_CAM_LCD_START_S);
	if(state == LCD_TRANSMIT_WORKING)
		ESP_LOGI(TAG_LCD_STATE, TAG_LCD_WORKING);
	return state;
}

void LcdInit(gpio_num_t pin[]){
	periph_module_enable(PERIPH_LCD_CAM_MODULE);
	SetLcdClock();
	MappingSignalAndGpio(pin);
	SetLcdInterrupts();
	DisableRgbMode();
	ConfigureCmdPhase();
	ConfigureDummyPhase();
	ConfigureDoutPhase();
	ConfigureCdSignalMode();
	UpdateLdcRegisters();
	ResetLcd();
}

void LcdStop(){
	// Start lcd
	WriteValueToAdress(LCD_CAM_LCD_USER_REG, LCD_CAM_LCD_START_S, LCD_CAM_LCD_START_M, LCD_CAM_LCD_START_IS_STOP);
}

void LcdStart(){
	// Start lcd
	WriteValueToAdress(LCD_CAM_LCD_USER_REG, LCD_CAM_LCD_START_S, LCD_CAM_LCD_START_M, LCD_CAM_LCD_START_IS_START);
}

void ResetLcd(){
	// Reset TX control unit (LCD_Ctrl) and Async Tx FIFO 
	WriteValueToAdress(LCD_CAM_LCD_USER_REG, LCD_CAM_LCD_RESET_S, LCD_CAM_LCD_RESET_M, LCD_CAM_LCD_RESET_IS_RESET);
	WriteValueToAdress(LCD_CAM_LCD_MISC_REG, LCD_CAM_LCD_AFIFO_RESET_S, LCD_CAM_LCD_AFIFO_RESET_M, LCD_CAM_LCD_AFIFO_RESET_IS_RESET);
}

void UpdateLdcRegisters(){
	// Set LCD_CAM_LCD_UPDATE.
	WriteValueToAdress(LCD_CAM_LCD_USER_REG, LCD_CAM_LCD_UPDATE_S, LCD_CAM_LCD_UPDATE_M, LCD_CAM_LCD_UPDATE_IS_UPDATE);
}

void ConfigureCdSignalMode(){
	// Dout not invert
	WriteValueToAdress(LCD_CAM_LCD_MISC_REG, LCD_CAM_LCD_CD_IDLE_EDGE_S, LCD_CAM_LCD_CD_IDLE_EDGE_M, LCD_CAM_LCD_CD_IDLE_EDGE_IS_HIGH);
	WriteValueToAdress(LCD_CAM_LCD_MISC_REG, LCD_CAM_LCD_CD_DATA_SET_S, LCD_CAM_LCD_CD_DATA_SET_M, LCD_CAM_LCD_CD_DATA_SET_NOT_INVERT);
}
void ConfigureDoutPhase(){
	//  Configure no dout phase. dout continuous output
	WriteValueToAdress(LCD_CAM_LCD_USER_REG, LCD_CAM_LCD_DOUT_S, LCD_CAM_LCD_DOUT_M, LCD_CAM_LCD_DOUT_ENABLE);
	WriteValueToAdress(LCD_CAM_LCD_USER_REG, LCD_CAM_LCD_ALWAYS_OUT_EN_S, LCD_CAM_LCD_ALWAYS_OUT_EN_M, LCD_CAM_LCD_ALWAYS_OUT_EN_CONTINUOS_MODE);
	WriteValueToAdress(LCD_CAM_LCD_USER_REG, LCD_CAM_LCD_2BYTE_EN_S, LCD_CAM_LCD_2BYTE_EN_M, LCD_CAM_LCD_2BYTE_EN_16_BIT);
	WriteValueToAdress(LCD_CAM_LCD_USER_REG, LCD_CAM_LCD_BYTE_ORDER_S, LCD_CAM_LCD_BYTE_ORDER_M, LCD_CAM_LCD_BYTE_ORDER_NOT_INVERT);
	WriteValueToAdress(LCD_CAM_LCD_USER_REG, LCD_CAM_LCD_BIT_ORDER_S, LCD_CAM_LCD_BIT_ORDER_M, LCD_CAM_LCD_BIT_ORDER_NOT_CHANGED);
	REG_WRITE(LCD_CAM_LCD_DATA_DOUT_MODE_REG, LCD_CAM_LCD_DATA_DOUT_MODE_DELAY_FALLING);
}

void ConfigureDummyPhase(){
	// Configure no dummy phase
	WriteValueToAdress(LCD_CAM_LCD_USER_REG, LCD_CAM_LCD_DUMMY_S, LCD_CAM_LCD_DUMMY_M, LCD_CAM_LCD_DUMMY_DISABLE);
}

void ConfigureCmdPhase(){
	// Configure no CMD phase
	WriteValueToAdress(LCD_CAM_LCD_USER_REG, LCD_CAM_LCD_CMD_S, LCD_CAM_LCD_CMD_M, LCD_CAM_LCD_CMD_DISABLE);
}

void DisableRgbMode(){
	 //Enable RGB mode, and input VSYNC, HSYNC, and DE signals
	 WriteValueToAdress(LCD_CAM_LCD_CTRL_REG, LCD_CAM_LCD_RGB_MODE_EN_S, LCD_CAM_LCD_RGB_MODE_EN_M, LCD_CAM_LCD_RGB_MODE_EN_DISABLE);
}

void SetLcdInterrupts(){
	//Triggered when the LCD transmitted all the data.
	WriteValueToAdress(LCD_CAM_LC_DMA_INT_ENA_REG, LCD_CAM_LCD_TRANS_DONE_INT_ENA_S, LCD_CAM_LCD_TRANS_DONE_INT_ENA_M, LCD_CAM_LCD_TRANS_DONE_INT_DISABLE);
}

void GpioConfig(uint32_t pin, uint32_t signal){
	WriteValueToAdress(GPIO_FUNCx_OUT_SEL_CFG_REG_ADDRESS(pin), GPIO_FUNC0_OUT_SEL_S, GPIO_FUNC0_OUT_SEL_M, signal);
	// Enable output driver cho GPIO gpioPin[i]
	WriteValueToAdress(GPIO_ENABLE_REG_ADDRESS(pin), GPIO_ENABLE_REG_BIT_S(pin), GPIO_ENABLE_REG_BIT_M(pin), GPIO_ENABLE_REG_ENABLE);
	// Configure output inversion for GPIO gpioPin[i]
	WriteValueToAdress(GPIO_FUNCx_OUT_SEL_CFG_REG_ADDRESS(pin), GPIO_FUNC0_OUT_INV_SEL_S, GPIO_FUNC0_OUT_INV_SEL_M, GPIO_FUNCx_OUT_INV_SEL_NOT_INVERT_OUTPUT);
	// Select source of output enable (OE) for GPIO gpioPin[i]
	WriteValueToAdress(GPIO_FUNCx_OUT_SEL_CFG_REG_ADDRESS(pin), GPIO_FUNC0_OEN_SEL_S, GPIO_FUNC0_OEN_SEL_M, GPIO_FUNCx_OEN_SEL_SIGNAL_PERIPHERAL);
	// Configure inversion of the output enable signal for GPIO gpioPin[i]
	WriteValueToAdress(GPIO_FUNCx_OUT_SEL_CFG_REG_ADDRESS(pin), GPIO_FUNC0_OEN_INV_SEL_S, GPIO_FUNC0_OEN_INV_SEL_M, GPIO_FUNCx_OEN_INV_SEL_NOT_INVERT_SIGNAL);

	// Select pad driver type for GPIO gpioPin[i]
	WriteValueToAdress(GPIO_PIN_REG(pin), GPIO_PIN0_PAD_DRIVER_S, GPIO_PIN0_PAD_DRIVER_M, GPIO_PINx_PAD_DRIVER_NORMAL);
	// Configure IO MUX so GPIO gpioPin[i] uses the GPIO function via GPIO matrix
	WriteValueToAdress(IO_MUX_x_REG(pin), MCU_SEL_S, MCU_SEL_M, IO_MUX_MCU_SEL_FUNC1);
	// Selected current flowing out
	WriteValueToAdress(IO_MUX_x_REG(pin), FUN_DRV_S, FUN_DRV_M, IO_MUX_FUN_DRV_10MA);
}

void MappingSignalAndGpio(gpio_num_t gpioPin[]){
	//Connected Gpio vs signal
	for(uint8_t i = 0; i < CLK_PIN; i++)
		GpioConfig( gpioPin[i], SIGNAL_NO_LCD_DATA_X(i));
	
	GpioConfig(gpioPin[CLK_PIN], LCD_PCLK_IDX);
	
	//DE mode is selected
	WriteValueToAdress(LCD_CAM_CAM_CTRL1_REG, LCD_CAM_CAM_VH_DE_MODE_EN_S, LCD_CAM_CAM_VH_DE_MODE_EN_M, LCD_CAM_CAM_VH_DE_MODE_EN_DE_MODE);
}

void SetLcdClock(){
	// Configure clock source -> Flcd_pclk = (clock source) / (N + b/a) * M0)
	WriteValueToAdress(LCD_CAM_LCD_CLOCK_REG, LCD_CAM_LCD_CK_OUT_EDGE_S, LCD_CAM_LCD_CK_OUT_EDGE_M, LCD_CAM_LCD_CK_OUT_EDGE_IS_FRIST_HALF_HIGH);
	WriteValueToAdress(LCD_CAM_LCD_CLOCK_REG, LCD_CAM_CLK_EN_S, LCD_CAM_CLK_EN_M, LCD_CAM_CLK_EN_S_DEFFAULT);
	WriteValueToAdress(LCD_CAM_LCD_CLOCK_REG, LCD_CAM_LCD_CLKM_DIV_NUM_S, LCD_CAM_LCD_CLKM_DIV_NUM_M, LCD_CAM_LCD_CLKM_DIV_NUM_DEFFAULT);
	WriteValueToAdress(LCD_CAM_LCD_CLOCK_REG, LCD_CAM_LCD_CLKM_DIV_A_S, LCD_CAM_LCD_CLKM_DIV_A_M, LCD_CAM_LCD_CLKM_DIV_A_DEFFAULT);
	WriteValueToAdress(LCD_CAM_LCD_CLOCK_REG, LCD_CAM_LCD_CLKM_DIV_B_S, LCD_CAM_LCD_CLKM_DIV_B_M, LCD_CAM_LCD_CLKM_DIV_B_DEFFAULT);
	WriteValueToAdress(LCD_CAM_LCD_CLOCK_REG, LCD_CAM_LCD_CLK_EQU_SYSCLK_S, LCD_CAM_LCD_CLK_EQU_SYSCLK_M, LCD_CAM_LCD_CLK_EQU_SYSCLK_DEFFAULT);
	WriteValueToAdress(LCD_CAM_LCD_CLOCK_REG, LCD_CAM_LCD_CLKCNT_N_S, LCD_CAM_LCD_CLKCNT_N_M, LCD_CAM_LCD_CLKCNT_N_DEFFAULT);	
	WriteValueToAdress(LCD_CAM_LCD_CLOCK_REG, LCD_CAM_LCD_CLK_SEL_S, LCD_CAM_LCD_CLK_SEL_M, LCD_CAM_LCD_CLK_SEL_DEFFAULT);
	//REG_SET_BIT(LCD_CAM_LCD_MISC_REG, LCD_CAM_LCD_UPDATE_IS_UPDATE);
}

