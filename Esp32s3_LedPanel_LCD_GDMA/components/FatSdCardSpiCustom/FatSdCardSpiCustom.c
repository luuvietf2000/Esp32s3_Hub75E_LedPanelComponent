
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FatSdCardSpiCustom.h"
#include "esp_heap_caps.h"
#include "ff.h"
#include "hal/spi_types.h"
#include "sys/dirent.h"

#define TAG_FAT_SD_CARD_SPI_CUSTOM_INIT														"FatSdCardSpiCustomInit"
#define FAT_SD_CARD_SPI_CUSTOM_INIT_FAIL_INITIALIZE_SPI_BUS_CONTENT							"Fat Sd Card Spi Custom Init Fail Cause Initialize SPI Bus"
#define FAT_SD_CARD_SPI_CUSTOM_INIT_FAIL_MOUNT_SD_CARD_CONTENT								"Failed to mount SD card: %s"
#define TAG_FAT_SD_CARD_SPI_CUSTOM															"FatSdCardSpiCustom"
#define FAT_SD_CARD_SPI_CUSTOM_OPEN_FAIL_CONTENT											"Open %s fail"
#define FAT_SD_CARD_SPI_CUSTOM_SEEK_FAIL_CONTENT											"Seek %s fail offset %lu"
#define FAT_SD_CARD_SPI_CUSTOM_READ_FAIL_CONTENT											"Read %s fail"
#define FAT_SD_CARD_SPI_CUSTOM_END_FILE_CONTENT												"End file %s"
#define FAT_SD_CARD_SPI_CUSTOM_READ_BYTE													"Read %lu byte"
#define FAT_SD_CARD_SPI_CUSTOM_READ_LIST_FILE_FAIL_CAUSE_OPEN_DERECTORY_FAIL_CONTENT		"FAT_SD_CARD_SPI_CUSTOM_READ_LIST_FILE_FAIL_CAUSE_OPEN_DERECTORY_FAIL"


void SetFileInfomationEmty(FileInfomation *fileInformation){
	*(fileInformation->path + FAT_SD_CARD_SPI_CUSTOM_INDEX_NAME_CHECK) = FAT_SD_CARD_SPI_CUSTOM_CHAR_EMPTY;
	fileInformation->offset = 0;
}

FileInfomationNameState FileInfomationNameCheck(FileInfomation *fileInformation){
	FileInfomationNameState state;
	state = *(fileInformation->path + FAT_SD_CARD_SPI_CUSTOM_INDEX_NAME_CHECK) == FAT_SD_CARD_SPI_CUSTOM_CHAR_EMPTY;
	return state ? FILE_INFOMATION_NAME_EMPTY : FILE_INFOMATION_NAME_NOT_EMPTY;
}

void FileInfomationInit(FileInfomation *fileInformation, uint32_t size){
	fileInformation->offset = 0;
	fileInformation->path = heap_caps_malloc(size * sizeof(char), MALLOC_CAP_DEFAULT);
	SetFileInfomationEmty(fileInformation);
}

FasrSdCardSpiCustomReadListFileState GetListFileSdCardSPI(char path[], DirentLinkerList *list){
	DirentLinkerListInit(list);
	DIR *dir = opendir(path);
	struct dirent *entry;

    if (dir == NULL)
    {
        ESP_LOGE(TAG_FAT_SD_CARD_SPI_CUSTOM, FAT_SD_CARD_SPI_CUSTOM_READ_LIST_FILE_FAIL_CAUSE_OPEN_DERECTORY_FAIL_CONTENT);
        return FAT_SD_CARD_SPI_CUSTOM_READ_LIST_FILE_FAIL_CAUSE_OPEN_DERECTORY_FAIL;
    }
	while ((entry = readdir(dir)) != NULL)
	{
		DirentNode *newNode = heap_caps_malloc(sizeof(DirentNode), MALLOC_CAP_DEFAULT);
		newNode->name = heap_caps_malloc(255 * sizeof(char), MALLOC_CAP_DEFAULT);
		newNode->next = NULL;
		memcpy(newNode->name, entry->d_name, strlen(entry->d_name) + 1);
		DirentLinkerListPush(list, newNode);
	}
    closedir(dir);
	
	return FAT_SD_CARD_SPI_CUSTOM_READ_LIST_FILE_OK;
}

void DirentLinkerListInit(DirentLinkerList *list){
	list->size = 0;
}

void DirentLinkerListPush(DirentLinkerList *list, DirentNode *entry){
	list->size++;
	if(list->size == 1){
		list->head = entry;
		return;
	}
	DirentNode *current = DirentLinkerListGetIndex(list, list->size - 1 - 1);

	current->next = entry;
}

void DirentLinkerListDetelte(DirentLinkerList *list){
	if(list->size == 0)
		return;
	DirentNode *current = list->head;
	DirentNode *pre = NULL;
	while(current != NULL){
		pre = current;
		current = current->next;
		free(pre->name);
		free(pre);
	}
	list->head = NULL;
	list->size = 0;
}
DirentNode* DirentLinkerListGetIndex(DirentLinkerList *list, uint32_t index){
	if(index >= list->size)
		return NULL;
	DirentNode *current = list->head;
	uint32_t count = 0;
	while(count++ < index){
		current = current->next;
	}
	return current;
}


FatSdCardSpiCustomCopyState CopySdCardSpiFile(char path[], uint8_t *des, uint32_t size, uint32_t offset){
	FILE *file = fopen(path, "rb");
    if (file == NULL) {
        ESP_LOGE(TAG_FAT_SD_CARD_SPI_CUSTOM, FAT_SD_CARD_SPI_CUSTOM_OPEN_FAIL_CONTENT, path);
        vTaskDelete(NULL);
        return FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_OPEN_FILE_FAIL;
    }
    
    fseek(file, offset, SEEK_SET);
    
    size_t totalBytes = 0;
    size_t readBytes = 0;
    
	while (totalBytes < size) {
	
	    size_t remain = size - totalBytes;
	
	    readBytes = fread(
	        des + totalBytes,
	        1,
	        remain,
	        file
	    );
	
	    if (readBytes == 0){
			fclose(file);
			ESP_LOGI(TAG_FAT_SD_CARD_SPI_CUSTOM, FAT_SD_CARD_SPI_CUSTOM_END_FILE_CONTENT, path);
	        return FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_FILE_END;
	    }
	
	    totalBytes += readBytes;
	}
	fclose(file);
	ESP_LOGI(TAG_FAT_SD_CARD_SPI_CUSTOM, FAT_SD_CARD_SPI_CUSTOM_READ_BYTE, (unsigned long) totalBytes);
	return FAT_SD_CARD_SPI_CUSTOM_COPY_OK;
}

esp_err_t FatSdCardSpiCustomInit(FatSdCardSpiCustomConfig *config){
	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	//host.slot = SPI3_HOST;
	config->host = host;
	config->host.max_freq_khz = 4000;
	esp_err_t ret = spi_bus_initialize(config->host.slot, &config->spiConfig, config->channelDma);
	if (ret != ESP_OK) {
	    ESP_LOGE(TAG_FAT_SD_CARD_SPI_CUSTOM_INIT, FAT_SD_CARD_SPI_CUSTOM_INIT_FAIL_INITIALIZE_SPI_BUS_CONTENT);
	    return ret;
    } 
    
    sdspi_device_config_t slotConfig = SDSPI_DEVICE_CONFIG_DEFAULT();
    slotConfig.gpio_cs = config->cs;
    slotConfig.host_id = config->host.slot;
    
    config->slotConfig = slotConfig;
    

    
    ret = esp_vfs_fat_sdspi_mount(FAT_SD_CARD_SPI_CUSTOM_MOUSNT_PATH,
    							  &config->host,
	                              &config->slotConfig,
	                              &config->mountConfig,
	                              &config->card); 
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_FAT_SD_CARD_SPI_CUSTOM_INIT, FAT_SD_CARD_SPI_CUSTOM_INIT_FAIL_MOUNT_SD_CARD_CONTENT, esp_err_to_name(ret));
        return ret;
    }
    sdmmc_card_print_info(stdout, config->card);
	return ret;
}