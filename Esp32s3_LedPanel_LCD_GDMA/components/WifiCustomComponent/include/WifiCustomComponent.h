#ifndef WIFI_CUSTOM_COMPONENT
#define WIFI_CUSTOM_COMPONENT

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

void WifiCustomInit(wifi_config_t *config, 
					uint32_t networkPart, 
					uint32_t subnetPart,
					uint32_t subnet, 
					uint32_t hostId);

#endif