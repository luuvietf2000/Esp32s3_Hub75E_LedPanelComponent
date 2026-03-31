#include "Hub75ELut.h"
#include <math.h>
#include <stdint.h>

static float lutGamma[HUB75E_LUT_COLOR][HUB75E_LUT_LEVEL];

void Hub75ELutInit(uint8_t bit, float gammaLut, float redScale, float greenScale, float blueScale){
	float red = GetScaleColorHub75E(redScale);
	float green = GetScaleColorHub75E(greenScale);
	float blue = GetScaleColorHub75E(blueScale);
	float gamma = GetGammaLutHub75E(gammaLut);
	
	for(uint32_t index = 0; index < HUB75E_LUT_COLOR; index++)
		for(uint32_t level = 0; level < HUB75E_LUT_LEVEL; level++){
			float normalized = (float) level / (HUB75E_LUT_LEVEL - 1);
			lutGamma[index][level] = pow(normalized, gamma);
		}
		
	Hub75EScaleLutGammaLedPanel(R_LUT_GAMMA_INDEX, red);
	Hub75EScaleLutGammaLedPanel(G_LUT_GAMMA_INDEX, green);
	Hub75EScaleLutGammaLedPanel(B_LUT_GAMMA_INDEX, blue);
	
	for(uint32_t index = 0; index < HUB75E_LUT_COLOR; index++)
		for(uint32_t level = 0; level < HUB75E_LUT_LEVEL; level++)
			lutGamma[index][level] = (uint8_t)round(lutGamma[index][level] / ((1 << HUB75E_RGB_COLOR_BIT_SINGLE) - 1) * ((1 << bit) - 1));
}

uint8_t HUB75ELutGetSingelColor(HUB75E_LUT_GAMMA_INDEX index, uint8_t level){
	return lutGamma[index][level];
}

void Hub75EScaleLutGammaLedPanel(uint32_t index, float scale){
	for(uint32_t level = 0; level < HUB75E_LUT_LEVEL; level++){
		lutGamma[index][level] *= scale;
	}
}

uint32_t Hub75ELutGetColor(uint32_t color, uint32_t bit){
	uint32_t red = Hub75EScaleBitGammaPixel(color, R_LUT_GAMMA_INDEX, HUB75E_RGB_RED_MASK, R_COLOR_S, bit);
	uint32_t green = Hub75EScaleBitGammaPixel(color, G_LUT_GAMMA_INDEX, HUB75E_RGB_GREEN_MASK, G_COLOR_S, bit);
	uint32_t blue = Hub75EScaleBitGammaPixel(color, B_LUT_GAMMA_INDEX, HUB75E_RGB_BLUE_MASK, B_COLOR_S, bit);
	return (red << R_COLOR_S) | (green << G_COLOR_S) | (blue << B_COLOR_S);
}

uint32_t Hub75EScaleBitGammaPixel(uint32_t color, uint32_t indexLutGamma, uint32_t mask, uint32_t start, uint32_t bit){
	uint32_t level = (color & mask) >> start;
	uint32_t totalPwm = (bit <= 0) ? 0 :((1 << bit) - 1);
	uint32_t newlevel = lutGamma[indexLutGamma][level] * totalPwm;
	return newlevel;
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