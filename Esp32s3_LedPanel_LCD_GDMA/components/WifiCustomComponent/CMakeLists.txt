idf_component_register(SRCS "WifiCustomComponent.c" "TcpCustom.c" "QueueMessageWifi.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi esp_netif esp_event nvs_flash MessageComponent)
