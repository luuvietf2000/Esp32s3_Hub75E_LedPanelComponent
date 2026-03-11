#ifndef FAT_SD_CARD_SPI_CUSTOM_H
#define FAT_SD_CARD_SPI_CUSTOM_H

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
	FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_READ_FAIL, 
	FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_SEEK_FAIL,
	FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_FILE_END
} FatSdCardSpiCustomCopyState;		

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

typedef struct DirentNode{
	struct DirentNode * next;
	char *name;
} DirentNode;

typedef struct DirentLinkerList{
	struct DirentNode *head; 
	uint32_t size;
} DirentLinkerList;

typedef struct FileInfomation{
	char *path;
	uint32_t offset;
} FileInfomation;

void SetFileInfomationEmty(FileInfomation *fileInformation);
FileInfomationNameState FileInfomationNameCheck(FileInfomation *fileInformation);
void FileInfomationInit(FileInfomation *fileInformation, uint32_t size);
FasrSdCardSpiCustomReadListFileState GetListFileSdCardSPI(char path[], DirentLinkerList *list);
void DirentLinkerListInit(DirentLinkerList *list);
void DirentLinkerListPush(DirentLinkerList *list, DirentNode *entry);
void DirentLinkerListDetelte(DirentLinkerList *list);
DirentNode* DirentLinkerListGetIndex(DirentLinkerList *list, uint32_t index);

FatSdCardSpiCustomCopyState CopySdCardSpiFile(char path[], uint8_t *des, uint32_t size, uint32_t offset);
esp_err_t FatSdCardSpiCustomInit(FatSdCardSpiCustomConfig *config);

#endif