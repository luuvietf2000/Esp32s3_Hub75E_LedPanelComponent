idf_component_register(SRCS "FatSdCardSpiCustom.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_spi sdmmc fatfs)

