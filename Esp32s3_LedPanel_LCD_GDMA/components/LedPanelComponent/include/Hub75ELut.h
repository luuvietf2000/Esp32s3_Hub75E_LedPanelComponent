#ifndef COMPONENTS_LEDPANELCOMPONENT_INCLUDE_HUB75ELUT_H_
#define COMPONENTS_LEDPANELCOMPONENT_INCLUDE_HUB75ELUT_H_

#include <stdint.h>
#define R_COLOR_S																16
#define G_COLOR_S																8
#define B_COLOR_S																0

#define HUB75E_LUT_LEVEL														256
#define HUB75E_LUT_COLOR														3

#define HUB75E_LUT_GAMMMA_MIN													0

#define HUB75E_LUT_SCALE_MIN													0
#define HUB75E_LUT_SCALE_MAX													1

#define HUB75E_RGB_RED_MASK														(0xFF << R_COLOR_S)
#define HUB75E_RGB_GREEN_MASK													(0xFF << G_COLOR_S)
#define HUB75E_RGB_BLUE_MASK													(0xFF << B_COLOR_S)

#define HUB75E_RGB_SINGLE_MASK													0xFF
#define HUB75E_RGB_COLOR_BIT_SINGLE												8

typedef enum{
	R_LUT_GAMMA_INDEX,
	G_LUT_GAMMA_INDEX,
	B_LUT_GAMMA_INDEX
}HUB75E_LUT_GAMMA_INDEX;

uint8_t HUB75ELutGetSingelColor(HUB75E_LUT_GAMMA_INDEX index, uint8_t level);

uint32_t Hub75EScaleBitGammaPixel(uint32_t color, uint32_t indexLutGamma, uint32_t mask, uint32_t start, uint32_t bit);

void Hub75EScaleLutGammaLedPanel(uint32_t index, float scale);

void Hub75ELutInit(uint8_t bit, float gammaLut, float redScale, float greenScale, float blueScale);

uint32_t Hub75ELutGetColor(uint32_t color, uint32_t bit);

float GetScaleColorHub75E(float scale);

float GetGammaLutHub75E(float gamma);
#endif /* COMPONENTS_LEDPANELCOMPONENT_INCLUDE_HUB75ELUT_H_ */
