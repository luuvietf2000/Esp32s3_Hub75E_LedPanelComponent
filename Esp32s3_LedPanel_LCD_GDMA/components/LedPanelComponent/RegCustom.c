#include "RegCustom.h"
#include "soc/soc.h"
#include <stdint.h>
#include "esp_log.h"

#define TAG_REG_READ 					"Read REG"
#define TAG_REG_WRITE 					"Write REG"
#define TAG_REG_CHECK_AFTER_REG			"Check Write REG"
#define PASS_CONTENT					"Pass"
#define NG_CONTENT						"Ng"
#define PASS_VALUE						1

uint32_t GetRegValue(uint32_t address, uint32_t mask, uint32_t start){
	uint32_t reg = (address & mask) >> start;
	return reg;
}

void WriteValueToAdress(uint32_t address, uint32_t start, uint32_t mask, uint32_t value){
	//ESP_LOGI(TAG_REG_READ ,"Address = 0x%08x, Bit start = %lu, value = %lu \n", (unsigned int) address, (unsigned long) start, (unsigned long) value);
	
	uint32_t current = REG_READ(address);
	//ESP_LOGI(TAG_REG_READ, "Address = 0x%08x, Value check = %lu \n", (unsigned int) address, (unsigned long) current);
	
	uint32_t reg = current;
	
	reg &= ~mask;
	reg |= (value << start) & mask;
	REG_WRITE(address, reg);
		/*
	uint32_t after = REG_READ(address);
	
	uint32_t isPass = reg == after;
	

	if(isPass == PASS_VALUE)
		ESP_LOGI(TAG_REG_CHECK_AFTER_REG, "Address = 0x%08x, write reg is pass\n", (unsigned int) address);
	else
		ESP_LOGE(TAG_REG_CHECK_AFTER_REG, "Address = 0x%08x, write reg is ng\n", (unsigned int) address);
		*/
 ;
	
}
