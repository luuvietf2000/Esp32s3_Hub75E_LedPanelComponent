#ifndef COMPONENTS_LEDPANELCOMPONENT_REGCUSTOM_H_
#define COMPONENTS_LEDPANELCOMPONENT_REGCUSTOM_H_

#include <stdint.h>

uint32_t GetRegValue(uint32_t address, uint32_t mask, uint32_t start);
void WriteValueToAdress(uint32_t address, uint32_t start, uint32_t mask, uint32_t value);

#endif /* COMPONENTS_LEDPANELCOMPONENT_REGCUSTOM_H_ */
