#include "Hub75ELut.h"
#include "esp_heap_caps.h"
#include <math.h>
#include <stdint.h>

uint16_t (*lutGamma)[HUB75E_LUT_LEVEL][HUB75E_RGB_COLOR_BIT_SINGLE];

void Hub75ELutInit(uint8_t red1S, uint8_t red2S, uint8_t green1S, uint8_t green2S, uint8_t blue1S, uint8_t blue2S ,uint8_t bit, float gammaLut, float redScale, float greenScale, float blueScale){
	float red = GetScaleColorHub75E(redScale);
	float green = GetScaleColorHub75E(greenScale);
	float blue = GetScaleColorHub75E(blueScale);
	float gamma = GetGammaLutHub75E(gammaLut);
	
	lutGamma = heap_caps_malloc(sizeof(uint16_t) * HUB75E_TABLE_LEVEL_PIN * HUB75E_LUT_LEVEL * HUB75E_RGB_COLOR_BIT_SINGLE, MALLOC_CAP_SPIRAM);
	
	float normalized, scaleBit;
	uint16_t gammaLevelRed, gammaLevelGreen, gammaLevelBlue;
	scaleBit = (1 << bit) - 1;
	for(uint32_t level = 0; level < HUB75E_LUT_LEVEL; level++){
		normalized = (float) level / (HUB75E_LUT_LEVEL - 1);
		gammaLevelRed = roundf(pow(normalized, gamma) * red * scaleBit);
		for(uint16_t i = 0; i < HUB75E_RGB_COLOR_BIT_SINGLE; i++){
			lutGamma[R1_PIN_INDEX][level][i] = ((gammaLevelRed >> i) & 1) << red1S;
			lutGamma[R2_PIN_INDEX][level][i] = ((gammaLevelRed >> i) & 1) << red2S;
		}
		
		gammaLevelGreen = roundf(pow(normalized, gamma) * green * scaleBit);
		for(uint16_t i = 0; i < HUB75E_RGB_COLOR_BIT_SINGLE; i++){
			lutGamma[G1_PIN_INDEX][level][i] = ((gammaLevelGreen >> i) & 1) << green1S;
			lutGamma[G2_PIN_INDEX][level][i] = ((gammaLevelGreen >> i) & 1) << green2S;
		}
		gammaLevelBlue = roundf(pow(normalized, gamma) * blue * scaleBit);
		for(uint16_t i = 0; i < HUB75E_RGB_COLOR_BIT_SINGLE; i++){
			lutGamma[B1_PIN_INDEX][level][i] = ((gammaLevelBlue >> i) & 1) << blue1S;
			lutGamma[B2_PIN_INDEX][level][i] = ((gammaLevelBlue >> i) & 1) << blue2S;
		}
	}
}


float GetGammaLutHub75E(float gamma){
	return gamma < HUB75E_LUT_GAMMMA_MIN ? HUB75E_LUT_GAMMMA_MIN : gamma;
}

float GetScaleColorHub75E(float scale){
	if(scale > HUB75E_LUT_SCALE_MIN && scale < HUB75E_LUT_SCALE_MAX)
		return scale;
	else if(scale < HUB75E_LUT_SCALE_MIN)
		return HUB75E_LUT_SCALE_MIN;
	else 
		return HUB75E_LUT_SCALE_MAX;
}