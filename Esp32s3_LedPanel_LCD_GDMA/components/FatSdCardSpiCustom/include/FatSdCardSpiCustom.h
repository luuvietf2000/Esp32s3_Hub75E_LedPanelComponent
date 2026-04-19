#ifndef FAT_SD_CARD_SPI_CUSTOM_H
#define FAT_SD_CARD_SPI_CUSTOM_H

#include "DirentLinkerListComponent.h"
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"
#include "hal/spi_types.h"
#include "sdmmc_cmd.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "soc/gpio_num.h"
#include <stdint.h>


#define FAT_SD_CARD_SPI_CUSTOM_END_FILE									0
#define FAT_SD_CARD_SPI_CUSTOM_MOUSNT_PATH 								"/sdcard"
#define FAT_SD_CARD_SPI_CUSTOM_INDEX_NAME_CHECK							0
#define FAT_SD_CARD_SPI_CUSTOM_CHAR_EMPTY								0

typedef enum{
	FAT_SD_CARD_SPI_CUSTOM_COPY_OK,
	FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_OPEN_FILE_FAIL,
	FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_READ_FILE_ERROR,
	FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_FILE_END
} FatSdCardSpiCustomCopyState;		

typedef enum{
    FAT_SD_CARD_SPI_CUSTOM_WRITE_OK,
    FAT_SD_CARD_SPI_CUSTOM_WRITE_FAIL_CAUSE_OPEN_FILE_FAIL,
    FAT_SD_CARD_SPI_CUSTOM_WRITE_FAIL_CAUSE_WRITE_FILE_ERROR,
    FAT_SD_CARD_SPI_CUSTOM_WRITE_FAIL_CAUSE_PARTIAL_WRITE,
    FAT_SD_CARD_SPI_CUSTOM_WRITE_FAIL_CAUSE_DISK_FULL
} FatSdCardSpiCustomWriteState;

typedef enum{
	FAT_SD_CARD_SPI_CUSTOM_READ_LIST_FILE_OK,
	FAT_SD_CARD_SPI_CUSTOM_READ_LIST_FILE_FAIL_CAUSE_OPEN_DERECTORY_FAIL
} FasrSdCardSpiCustomReadListFileState;

typedef enum{
	FILE_INFOMATION_NAME_EMPTY,
	FILE_INFOMATION_NAME_NOT_EMPTY
} FileInfomationNameState;

typedef struct FatSdCardSpiCustomConfig{
	spi_bus_config_t spiConfig;
	sdmmc_host_t host;
	uint32_t channelDma;
	gpio_num_t cs;
	sdspi_device_config_t slotConfig;
	esp_vfs_fat_sdmmc_mount_config_t mountConfig;
	sdmmc_card_t *card;
} FatSdCardSpiCustomConfig;

typedef struct FileInfomation{
	char *path;
	uint32_t offset;
} FileInfomation;

void GetSdCardInfo(uint64_t *total, uint64_t *free, uint64_t *used);
void SetFileInfomationEmty(FileInfomation *fileInformation);
FileInfomationNameState FileInfomationNameCheck(FileInfomation *fileInformation);
void FileInfomationInit(FileInfomation *fileInformation, uint32_t size);
FasrSdCardSpiCustomReadListFileState GetListFileSdCardSPI(char path[], DirentLinkerList *list);
FatSdCardSpiCustomWriteState WriteSdCardSpiFileOptimized(FILE *file, uint8_t *buffer, uint32_t size);
FatSdCardSpiCustomCopyState CopySdCardSpiFile(FILE *file, uint8_t *buffer, uint32_t size);
esp_err_t FatSdCardSpiCustomInit(FatSdCardSpiCustomConfig *config);

#endif