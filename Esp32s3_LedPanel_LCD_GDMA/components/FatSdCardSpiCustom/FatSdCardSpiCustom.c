
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
#define FAT_SD_CARD_SPI_CUSTOM_OPEN_FAIL_CONTENT											"Open fail"
#define FAT_SD_CARD_SPI_CUSTOM_SEEK_FAIL_CONTENT											"Seek fail offset %lu"
#define FAT_SD_CARD_SPI_CUSTOM_READ_FAIL_CONTENT											"Read %s fail"
#define FAT_SD_CARD_SPI_CUSTOM_END_FILE_CONTENT												"End file %s"
#define FAT_SD_CARD_SPI_CUSTOM_READ_BYTE													"Read %lu byte"
#define FAT_SD_CARD_SPI_CUSTOM_READ_LIST_FILE_FAIL_CAUSE_OPEN_DERECTORY_FAIL_CONTENT		"FAT_SD_CARD_SPI_CUSTOM_READ_LIST_FILE_FAIL_CAUSE_OPEN_DERECTORY_FAIL"


void GetSdCardInfo(uint64_t *total, uint64_t *free, uint64_t *used) {
    FATFS *fs;
    DWORD free_clusters;

    FRESULT res = f_getfree("0:", &free_clusters, &fs);

    if (res != FR_OK) {
        ESP_LOGE("SD", "f_getfree failed: %d", res);
        return;
    }

    DWORD total_clusters = fs->n_fatent - 2;

    *total = (uint64_t) total_clusters * fs->csize * 512;
    *free  = (uint64_t) free_clusters  * fs->csize * 512;
    *used  = *total - *free;

    ESP_LOGI("SD", "Total: %llu MB", *total / (1024 * 1024));
    ESP_LOGI("SD", "Free : %llu MB", *free  / (1024 * 1024));
    ESP_LOGI("SD", "Used : %llu MB", *used  / (1024 * 1024));
}

FatSdCardSpiCustomWriteState WriteSdCardSpiFileOptimized(FILE *file, uint8_t *buffer, uint32_t size){
    if (file == NULL) {
        ESP_LOGE(TAG_FAT_SD_CARD_SPI_CUSTOM, "WRITE FILE OPEN FAIL");
        return FAT_SD_CARD_SPI_CUSTOM_WRITE_FAIL_CAUSE_OPEN_FILE_FAIL;
    }

    size_t written = fwrite(buffer, 1, size, file);

    if (written != size){
        if (ferror(file)){
            ESP_LOGE(TAG_FAT_SD_CARD_SPI_CUSTOM, "WRITE FILE ERROR");
            return FAT_SD_CARD_SPI_CUSTOM_WRITE_FAIL_CAUSE_WRITE_FILE_ERROR;
        }

        if (written == 0){
            ESP_LOGE(TAG_FAT_SD_CARD_SPI_CUSTOM, "DISK FULL");
            return FAT_SD_CARD_SPI_CUSTOM_WRITE_FAIL_CAUSE_DISK_FULL;
        }

        ESP_LOGE(TAG_FAT_SD_CARD_SPI_CUSTOM, "PARTIAL WRITE");
        return FAT_SD_CARD_SPI_CUSTOM_WRITE_FAIL_CAUSE_PARTIAL_WRITE;
    }

    ESP_LOGI(TAG_FAT_SD_CARD_SPI_CUSTOM, "WRITE BYTE: %lu", (unsigned long)written);
    return FAT_SD_CARD_SPI_CUSTOM_WRITE_OK;
}

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
	DIR *dir = opendir(path);
	struct dirent *entry;

    if (dir == NULL)
    {
        ESP_LOGE(TAG_FAT_SD_CARD_SPI_CUSTOM, FAT_SD_CARD_SPI_CUSTOM_READ_LIST_FILE_FAIL_CAUSE_OPEN_DERECTORY_FAIL_CONTENT);
        return FAT_SD_CARD_SPI_CUSTOM_READ_LIST_FILE_FAIL_CAUSE_OPEN_DERECTORY_FAIL;
    }
	while ((entry = readdir(dir)) != NULL)
	{
		DirentNode *newNode = DirentLinkerListGetIndex(list, list->size++);
		memcpy(newNode->buffer, entry->d_name, strlen(entry->d_name) + 1);
	}
	
    closedir(dir);
	
	return FAT_SD_CARD_SPI_CUSTOM_READ_LIST_FILE_OK;
}


FatSdCardSpiCustomCopyState CopySdCardSpiFile(FILE *file, uint8_t *buffer, uint32_t size){
	if (file == NULL) {
        ESP_LOGE(TAG_FAT_SD_CARD_SPI_CUSTOM, FAT_SD_CARD_SPI_CUSTOM_OPEN_FAIL_CONTENT);
        return FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_OPEN_FILE_FAIL;
    }
	size_t totalBytes = 0;
    size_t readBytes = 0;
	while (totalBytes < size) {
	    size_t remain = size - totalBytes;
	    readBytes = fread(
	        buffer + totalBytes,
	        1,
	        remain,
	        file
	    );
	
	    if (readBytes == 0){
			if(feof(file)){
				ESP_LOGI(TAG_FAT_SD_CARD_SPI_CUSTOM, FAT_SD_CARD_SPI_CUSTOM_END_FILE_CONTENT);
				return FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_FILE_END;
			} else if(ferror(file)){
				ESP_LOGE(TAG_FAT_SD_CARD_SPI_CUSTOM, "FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_READ_FILE_ERROR");
				return FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_READ_FILE_ERROR;
			}
			clearerr(file);
	    }
	
	    totalBytes += readBytes;
	}
	ESP_LOGI(TAG_FAT_SD_CARD_SPI_CUSTOM, FAT_SD_CARD_SPI_CUSTOM_READ_BYTE, (unsigned long) totalBytes);
	return FAT_SD_CARD_SPI_CUSTOM_COPY_OK;
}

esp_err_t FatSdCardSpiCustomInit(FatSdCardSpiCustomConfig *config){
	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	//host.slot = SPI3_HOST;
	config->host = host;
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