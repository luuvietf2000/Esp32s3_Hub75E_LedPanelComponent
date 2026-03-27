#include <stdint.h>
#include <stdio.h>
#include "WifiCustomComponent.h"
#include "lwip/sockets.h"

#define TAG_WIFI_CUSTOM														"WifiCustom"


void WifiCustomInit(wifi_config_t *config, 
					uint32_t networkPart, 
					uint32_t subnetPart,
					uint32_t subnet, 
					uint32_t hostId)
{
	esp_netif_init();
    esp_event_loop_create_default();
 	esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();

/*
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, networkPart, subnetPart, subnet, hostId);
    IP4_ADDR(&ip_info.gw, networkPart, subnetPart, subnet, hostId);
    IP4_ADDR(&ip_info.netmask, 255,255,255,0);
    
    esp_netif_dhcps_stop(ap_netif);
    esp_netif_set_ip_info(ap_netif, &ip_info);
    esp_netif_dhcps_start(ap_netif);
    */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    if (strlen((char*)config->ap.password) == 0) {
        config->ap.authmode = WIFI_AUTH_OPEN;
    }
    /*
	esp_netif_tx_rx_event_enable(ap_netif);
	
	ESP_ERROR_CHECK(
	    esp_event_handler_instance_register(
	        IP_EVENT,
	        IP_EVENT_TX_RX,
	        function,
	        NULL,
	        NULL
    	)
	);
 	*/
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_WIFI_CUSTOM, "AP started. SSID:%s password:%s",
             config->ap.ssid, config->ap.password);
}