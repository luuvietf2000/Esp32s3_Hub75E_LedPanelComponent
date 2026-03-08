#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "LEDPANELComponent.h"
#include "QueueImageRaw.h"
#include "driver/spi_common.h"
#include "esp_heap_caps.h"
#include "ff.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/spi_types.h"
#include "portmacro.h"
#include "soc/gpio_num.h"
#include "FatSdCardSpiCustom.h"
#include "sys/dirent.h"

#define LEDPANEL_WIDTH 						128
#define LEDPANEL_HEIGTH 					64

#define LEDPANEL_PIN_A          			GPIO_NUM_17
#define LEDPANEL_PIN_B          			GPIO_NUM_18
#define LEDPANEL_PIN_C          			GPIO_NUM_8
#define LEDPANEL_PIN_D          			GPIO_NUM_9
#define LEDPANEL_PIN_E          			GPIO_NUM_14

#define LEDPANEL_PIN_R1         			GPIO_NUM_4
#define LEDPANEL_PIN_G1         			GPIO_NUM_5
#define LEDPANEL_PIN_B1         			GPIO_NUM_6
#define LEDPANEL_PIN_R2         			GPIO_NUM_7
#define LEDPANEL_PIN_G2         			GPIO_NUM_15
#define LEDPANEL_PIN_B2         			GPIO_NUM_16

#define LEDPANEL_PIN_CLK        			GPIO_NUM_1
#define LEDPANEL_PIN_LATCH      			GPIO_NUM_2
#define LEDPANEL_PIN_OE			 			GPIO_NUM_21

#define SD_CARD_MOSI						GPIO_NUM_11
#define SD_CARD_MISO						GPIO_NUM_13
#define SD_CARD_CLK							GPIO_NUM_12
#define SD_CARD_CS							GPIO_NUM_10
#define SD_CARD_OPEN_MAX_FILE				5

//-------------------------------------------------------------------------------/
#define SECOND_UINT							1000
#define DIRECTORY_NAME						"ImageRaw"
#define PATH_DIR_IMAGE_RAW					FAT_SD_CARD_SPI_CUSTOM_MOUSNT_PATH "/" DIRECTORY_NAME
#define SIZE_NAME							256
#define BUFFER_SIZE							4096
//-------------------------------------------------------------------------------/
#define PATH_FILE_TEST						"/sdcard/ImageRaw/test.imgRaw"
#define TAG_APP_MAIN						"app_main"
#define APP_MAIN_INIT						"app_main init"
#define TAG_LEDPANEL_TASK					"LedPanelTask"
#define TAG_CONVERT_IMAGE_TASK				"CoverntImageToVectorGdmadescriptorsNodeTask"
#define TAG_SD_CARD_SPI_TASK				"SdCardSpiTask"
#define TASK_START							"Task start on core %d"
#define TASK_RUNNING						"Task running on core %d"
#define TAG_DELAY							"Task delay %d ms"
#define SD_OPEN_FAIL						"Open %s fail"
#define TASK_NOTYFI							"Task notify cause %s"
//-------------------------------------------------------------------------------/

typedef enum{
	FREE_RTOS_PRIORITY_LOW 			= 5,
	FREE_RTOS_PRIORITY_MEDIUM 		= 10,
	FREE_RTOS_PRIORITY_HIGH			= 15,
	FREE_RTOS_PRIORITY_REAL_TIME	= 20
} FreeRtosPriorityEnum;

typedef enum{
	CORE_0,
	CORE_1
} CoreEnum;

//-------------------------------------------------------------------------------/

const char pathImageRawFolder[] = FAT_SD_CARD_SPI_CUSTOM_MOUSNT_PATH "/" PATH_DIR_IMAGE_RAW;
LedPanelConfig ledPanelConfig;

FatSdCardSpiCustomConfig sdCardConfig;

SemaphoreHandle_t queueImageRawMutex;
SemaphoreHandle_t queueGdmaDescriptorsMutex;
uint32_t fps;

uint32_t timeLedPanelUpdate;
//-------------------------------------------------------------------------------/
void RequestNextGdmaDescriptorsNodeInQueue();
void PushGdmaDescriptorsNode();
void PushImageRawInQueue();
void RandomImageRawName(DIR *dir, char name[]);
void LedPanelConfigInit(LedPanelConfig *config);
void FatSdCardSpiCustomConfigInit(FatSdCardSpiCustomConfig *config);

void LedPanelTask(void *pvParameters);
void CoverntImageToVectorGdmadescriptorsNodeTask(void *pvParameters);
void SdCardSpiTask(void *pvParameters);

void SemaphoreInit();
void CreateTask();

void app_main(void)
{
	ESP_LOGI(TAG_APP_MAIN, APP_MAIN_INIT);
	FatSdCardSpiCustomConfigInit(&sdCardConfig);
	LedPanelConfigInit(&ledPanelConfig);
	QueueImageRawStateEnum state;
	uint32_t index = 0;
	FatSdCardSpiCustomCopyState sdCardCopyState;

	uint32_t length =  ledPanelConfig.style.heigth * ledPanelConfig.style.width * HUB75E_LUT_COLOR;
	while((state = GetQueueImageRawState())  != QUEUE_IMAGE_RAW_FULL) {
		PushQueuueImageRaw();
		uint8_t *des = PeekTailQueueImageRaw();
		sdCardCopyState = CopySdCardSpiFile(PATH_FILE_TEST, des, length, index);
		if(sdCardCopyState != FAT_SD_CARD_SPI_CUSTOM_COPY_OK)
			break;
		index += length;
	}
	
	while (1){
		PopQueueImageRaw();
		uint8_t *buffer = PeekHeadQueueImageRaw();
		QueueVectorGdmaDescriptorsNodePush(&ledPanelConfig, buffer);
		RequestNextVectorGdmaDescriptorsNode(&ledPanelConfig);
		vTaskDelay(pdMS_TO_TICKS(15));
	}
	
}

//-------------------------------------------------------------------------------/

void CreateTask(){
	xTaskCreatePinnedToCore(
        LedPanelTask,
        "LedPanelTask",
        4096,
        NULL,
        FREE_RTOS_PRIORITY_REAL_TIME,
        NULL,
        CORE_1
    );
    xTaskCreatePinnedToCore(
        CoverntImageToVectorGdmadescriptorsNodeTask,
        "CoverntImageToVectorGdmadescriptorsNodeTask",
        8192,
        NULL,
        FREE_RTOS_PRIORITY_HIGH,
        NULL,
        CORE_1
    );
    xTaskCreatePinnedToCore(
        SdCardSpiTask,
        "SdCardSpiTask",
        16392,
        NULL,
        FREE_RTOS_PRIORITY_MEDIUM,
        NULL,
        CORE_1
    );
}

void LedPanelTask(void *pvParameters){
	ESP_LOGI(TAG_LEDPANEL_TASK, TASK_START, xPortGetCoreID());
	while (1) {
		ESP_LOGI(TAG_LEDPANEL_TASK, TASK_RUNNING, xPortGetCoreID());
		RequestNextGdmaDescriptorsNodeInQueue();
		ESP_LOGI(TAG_LEDPANEL_TASK, TAG_DELAY, timeLedPanelUpdate);
		//vTaskDelay(pdMS_TO_TICKS(timeLedPanelUpdate));
		vTaskDelay(pdMS_TO_TICKS(10000));
	}
}


void CoverntImageToVectorGdmadescriptorsNodeTask(void *pvParameters){
	ESP_LOGI(TAG_CONVERT_IMAGE_TASK, TASK_START, xPortGetCoreID());
	while (1) {
		ESP_LOGI(TAG_CONVERT_IMAGE_TASK, TASK_RUNNING, xPortGetCoreID());
		if(xSemaphoreTake(queueGdmaDescriptorsMutex, portMAX_DELAY) == pdTRUE){
			QueueVectorGdmaDescriptorsNodeState state = CheckQueueVectorGdmaDescriptorsNodeState();
			if(state != QUEUE_VECTOR_DESCRIPTIORS_FULL){
				PushGdmaDescriptorsNode();
			}
			xSemaphoreGive(queueGdmaDescriptorsMutex);
		}
		
		vTaskDelay(pdMS_TO_TICKS(10000));
	}
}

void SdCardSpiTask(void *pvParameters){
	ESP_LOGI(TAG_SD_CARD_SPI_TASK, TASK_START, xPortGetCoreID());
	while (1) {
		ESP_LOGI(TAG_SD_CARD_SPI_TASK, TASK_RUNNING, xPortGetCoreID());
		PushImageRawInQueue();
		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}


//-------------------------------------------------------------------------------/

void RequestNextGdmaDescriptorsNodeInQueue(){
	if(xSemaphoreTake(queueGdmaDescriptorsMutex, portMAX_DELAY) == pdTRUE){
		QueueVectorGdmaDescriptorsNodeState state = CheckQueueVectorGdmaDescriptorsNodeState();
		if(state != QUEUE_VECTOR_DESCRIPTIORS_EMPTY){
			RequestNextVectorGdmaDescriptorsNode(&ledPanelConfig);
		}
		xSemaphoreGive(queueGdmaDescriptorsMutex);
	}
}

void PushGdmaDescriptorsNode(){
	if (xSemaphoreTake(queueImageRawMutex, portMAX_DELAY) == pdTRUE){
		QueueImageRawStateEnum state = GetQueueImageRawState();
		if(state != QUEUE_IMAGE_RAW_EMPTY){
			PopQueueImageRaw();
			uint8_t *des = PeekHeadQueueImageRaw();
			QueueVectorGdmaDescriptorsNodePush(&ledPanelConfig, des);
		}
		xSemaphoreGive(queueImageRawMutex);
	}
}

void PushImageRawInQueue(){
	if (xSemaphoreTake(queueImageRawMutex, portMAX_DELAY) == pdTRUE){
		QueueImageRawStateEnum state = GetQueueImageRawState();
		if(state != QUEUE_IMAGE_RAW_FULL){
			PushQueuueImageRaw();
			uint8_t *des = PeekTailQueueImageRaw();
			CopySdCardSpiFile(PATH_FILE_TEST, des, 4096, 0);
		}
		xSemaphoreGive(queueImageRawMutex);
	}
}


void RandomImageRawName(DIR *dir, char name[]){
	DirentLinkerList list;
	GetListFileSdCardSPI(PATH_DIR_IMAGE_RAW, &list);
  	for(uint32_t i = 0; i < list.size; i++){
		  DirentNode *node = DirentLinkerListGetIndex(&list, i);
		  ESP_LOGI("list", "%s", node->name);
	}
	DirentLinkerListDetelte(&list);
}

void FatSdCardSpiCustomConfigInit(FatSdCardSpiCustomConfig *config){
	 spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_CARD_MOSI,
        .miso_io_num = SD_CARD_MISO,
        .sclk_io_num = SD_CARD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = BUFFER_SIZE,
    };
    config->cs = SD_CARD_CS;
    config->spiConfig = bus_cfg;
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = SD_CARD_OPEN_MAX_FILE,
        .allocation_unit_size = 16 * 1024
    };
    config->mountConfig = mount_config;
    config->channelDma = SPI_DMA_CH_AUTO;
    FatSdCardSpiCustomInit(config);
    
}

void LedPanelConfigInit(LedPanelConfig *config){
	config->style.width = LEDPANEL_WIDTH;
	config->style.heigth = LEDPANEL_HEIGTH;
	config->style.scan = LEDPANEL_SCAN_2_PART;
	config->style.color = LEDPANEL_RGB888;
	config->style.gamma = 2.2;
	config->style.redScale = 1;
	config->style.greenScale = 0.8;
	config->style.blueScale= 1;
	
	gpio_num_t ledPanelPin[] = {
		LEDPANEL_PIN_R1, LEDPANEL_PIN_G1, LEDPANEL_PIN_B1,
		LEDPANEL_PIN_R2, LEDPANEL_PIN_G2, LEDPANEL_PIN_B2,
		LEDPANEL_PIN_A, LEDPANEL_PIN_B, LEDPANEL_PIN_C, LEDPANEL_PIN_D, LEDPANEL_PIN_E,
		 LEDPANEL_PIN_LATCH, LEDPANEL_PIN_OE, LEDPANEL_PIN_CLK
	};
	
	LedPanelInit(config, ledPanelPin, 2, 200);
}

void SemaphoreInit(){
	queueImageRawMutex = xSemaphoreCreateMutex();
	queueGdmaDescriptorsMutex = xSemaphoreCreateMutex();
}
