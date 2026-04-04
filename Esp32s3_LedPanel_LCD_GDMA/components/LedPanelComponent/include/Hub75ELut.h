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

#define HUB75E_TABLE_LEVEL_PIN													6

typedef enum{
	R_LUT_GAMMA_INDEX,
	G_LUT_GAMMA_INDEX,
	B_LUT_GAMMA_INDEX
}HUB75E_LUT_GAMMA_INDEX;

typedef enum{
	R1_PIN_INDEX,
	R2_PIN_INDEX,
	G1_PIN_INDEX,
	G2_PIN_INDEX,
	B1_PIN_INDEX,
	B2_PIN_INDEX,
} HUB75E_LUT_GAMMA_PIN_INDEX;


extern uint16_t (*lutGamma)[HUB75E_LUT_LEVEL][HUB75E_RGB_COLOR_BIT_SINGLE];

void Hub75ELutInit(uint8_t red1S, uint8_t red2S, uint8_t green1S, uint8_t green2S, uint8_t blue1S, uint8_t blue2S ,uint8_t bit, float gammaLut, float redScale, float greenScale, float blueScale);

float GetScaleColorHub75E(float scale);

float GetGammaLutHub75E(float gamma);
#endif /* COMPONENTS_LEDPANELCOMPONENT_INCLUDE_HUB75ELUT_H_ */
