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
#include "esp_random.h"

#define LEDPANEL_WIDTH 																128
#define LEDPANEL_HEIGTH 															64

#define LEDPANEL_PIN_A          													GPIO_NUM_17
#define LEDPANEL_PIN_B          													GPIO_NUM_18
#define LEDPANEL_PIN_C          													GPIO_NUM_8
#define LEDPANEL_PIN_D          													GPIO_NUM_9
#define LEDPANEL_PIN_E          													GPIO_NUM_14

#define LEDPANEL_PIN_R1         													GPIO_NUM_4
#define LEDPANEL_PIN_G1         													GPIO_NUM_5
#define LEDPANEL_PIN_B1         													GPIO_NUM_6
#define LEDPANEL_PIN_R2        											 			GPIO_NUM_7
#define LEDPANEL_PIN_G2         													GPIO_NUM_15
#define LEDPANEL_PIN_B2         													GPIO_NUM_16

#define LEDPANEL_PIN_CLK        													GPIO_NUM_1
#define LEDPANEL_PIN_LATCH      													GPIO_NUM_2
#define LEDPANEL_PIN_OE			 													GPIO_NUM_21

#define SD_CARD_MOSI																GPIO_NUM_11
#define SD_CARD_MISO																GPIO_NUM_13
#define SD_CARD_CLK																	GPIO_NUM_12
#define SD_CARD_CS																	GPIO_NUM_10
#define SD_CARD_OPEN_MAX_FILE														5

//-------------------------------------------------------------------------------/
#define SECOND_UINT																	1000
#define DIRECTORY_NAME																"ImageRaw"
#define PATH_DIR_IMAGE_RAW															FAT_SD_CARD_SPI_CUSTOM_MOUSNT_PATH "/" DIRECTORY_NAME "/"
#define SIZE_NAME																	256
#define BUFFER_SIZE																	4096

//-------------------------------------------------------------------------------/
#define PATH_FILE_TEST																"/sdcard/ImageRaw/test.imgRaw"
#define TAG_APP_MAIN																"app_main"
#define APP_MAIN_INIT																"app_main init"
#define TAG_LEDPANEL_TASK															"LedPanelTask"
#define TAG_CONVERT_IMAGE_TASK														"CoverntImageToVectorGdmadescriptorsNodeTask"
#define TAG_SD_CARD_SPI_TASK														"SdCardSpiTask"
#define TASK_START																	"Task start on core %d"
#define TASK_RUNNING																"Task running on core"
#define TAG_DELAY																	"Task delay %d ms"
#define SD_OPEN_FAIL																"Open %s fail"
#define TASK_NOTYFI																	"Task notify cause %s"
#define RANDOM_IMAGE_RAW_NAME														"RandomImageRawName"
#define RANDOM_IMAGE_RAW_FAIL_CAUSE_DIRECTORY_EMPTY_CONTENT 						"RANDOM_IMAGE_RAW_FAIL_CAUSE_DIRECTORY_EMPTY_CONTENT"		
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

typedef enum{
	RANDOM_IMAGE_RAW_NAME_OK,
	RANDOM_IMAGE_RAW_NAME_FAIL_CAUSE_DIRECTORY_EMPTY
} RandomImageRawNameState;

//-------------------------------------------------------------------------------/


//-------------------------------------------------------------------------------/

const char pathImageRawFolder[] = FAT_SD_CARD_SPI_CUSTOM_MOUSNT_PATH "/" PATH_DIR_IMAGE_RAW;
LedPanelConfig ledPanelConfig;

FatSdCardSpiCustomConfig sdCardConfig;

SemaphoreHandle_t queueImageRawMutex;
SemaphoreHandle_t queueGdmaDescriptorsMutex;
uint32_t fps;
struct FileInfomation ledPanelFile; 
uint32_t timeLedPanelUpdate;
TaskHandle_t CoverntImageToVectorGdmadescriptorsNodeHandle = NULL;
//-------------------------------------------------------------------------------/
void RequestNextGdmaDescriptorsNodeInQueue();
void TaskNotify(TaskHandle_t *task);
RandomImageRawNameState RandomImageRawName(char *path);
void LedPanelConfigInit(LedPanelConfig *config);
void FatSdCardSpiCustomConfigInit(FatSdCardSpiCustomConfig *config);

void LedPanelTask(void *pvParameters);
void CoverntImageToVectorGdmadescriptorsNodeTask(void *pvParameters);
void SdCardSpiTask(void *pvParameters);

void SemaphoreInit();
void CreateTask();
void Setup();

void app_main(void)
{
	Setup();
	ESP_LOGI(TAG_APP_MAIN, APP_MAIN_INIT);
	FatSdCardSpiCustomConfigInit(&sdCardConfig);
	LedPanelConfigInit(&ledPanelConfig);
	FileInfomationInit(&ledPanelFile, SIZE_NAME);
	SemaphoreInit();
	CreateTask();
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
        &CoverntImageToVectorGdmadescriptorsNodeHandle,
        CORE_1
    );
    xTaskCreatePinnedToCore(
        SdCardSpiTask,
        "SdCardSpiTask",
        8192,
        NULL,
        FREE_RTOS_PRIORITY_MEDIUM,
        NULL,
        CORE_1
    );
}

void LedPanelTask(void *pvParameters){
	ESP_LOGI(TAG_LEDPANEL_TASK, TASK_START, xPortGetCoreID());
	while (1) {
		ESP_LOGI(TAG_LEDPANEL_TASK, TASK_RUNNING);
		if(xSemaphoreTake(queueGdmaDescriptorsMutex, portMAX_DELAY) == pdTRUE){
			RequestNextGdmaDescriptorsNodeInQueue();
			xSemaphoreGive(queueGdmaDescriptorsMutex);
		}
		//vTaskDelay(pdMS_TO_TICKS(timeLedPanelUpdate));
		vTaskDelay(pdMS_TO_TICKS(SECOND_UINT/fps));
	}
}


void CoverntImageToVectorGdmadescriptorsNodeTask(void *pvParameters){
	ESP_LOGI(TAG_CONVERT_IMAGE_TASK, TASK_START, xPortGetCoreID());
	QueueImageRawStateEnum state;
	eTaskState taskState;
	while (1) {
		ESP_LOGI(TAG_CONVERT_IMAGE_TASK, TASK_RUNNING);
		taskState = eReady;
		if(xSemaphoreTake(queueImageRawMutex, portMAX_DELAY) == pdTRUE){
			if((state = GetQueueImageRawState()) != QUEUE_IMAGE_RAW_EMPTY){
				if(xSemaphoreTake(queueGdmaDescriptorsMutex, portMAX_DELAY)){
					QueueVectorGdmaDescriptorsNodeState state = CheckQueueVectorGdmaDescriptorsNodeState();
					if(state != QUEUE_VECTOR_DESCRIPTIORS_FULL){
						PopQueueImageRaw();
						uint8_t *buffer = PeekHeadQueueImageRaw();
						QueueVectorGdmaDescriptorsNodePush(&ledPanelConfig, buffer);
					}
					if(CheckQueueVectorGdmaDescriptorsNodeState() == QUEUE_VECTOR_DESCRIPTIORS_FULL)
						taskState = eBlocked;
					xSemaphoreGive(queueGdmaDescriptorsMutex);
				}
			}
			if((state = GetQueueImageRawState()) == QUEUE_IMAGE_RAW_EMPTY)
				taskState = eBlocked;		
			xSemaphoreGive(queueImageRawMutex);
		}
		if(taskState == eBlocked)
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		taskYIELD();
	}
}

void SdCardSpiTask(void *pvParameters){
	ESP_LOGI(TAG_SD_CARD_SPI_TASK, TASK_START, xPortGetCoreID());
	QueueImageRawStateEnum state;
	FatSdCardSpiCustomCopyState sdCardCopyState = FAT_SD_CARD_SPI_CUSTOM_COPY_OK;
	const uint32_t length =  ledPanelConfig.style.heigth * ledPanelConfig.style.width * HUB75E_LUT_COLOR;
	uint8_t *des;
	while (1) {
		ESP_LOGI(TAG_SD_CARD_SPI_TASK, TASK_RUNNING);
		if(xSemaphoreTake(queueImageRawMutex, portMAX_DELAY) == pdTRUE){
			if((state = GetQueueImageRawState()) != QUEUE_IMAGE_RAW_FULL){
				
				if(sdCardCopyState == FAT_SD_CARD_SPI_CUSTOM_COPY_OK)
					PushQueuueImageRaw();
				des = PeekTailQueueImageRaw();
				xSemaphoreGive(queueImageRawMutex);
							
				if(FileInfomationNameCheck(&ledPanelFile) == FILE_INFOMATION_NAME_EMPTY)
					RandomImageRawName(ledPanelFile.path);
					
				sdCardCopyState = CopySdCardSpiFile(ledPanelFile.path, des, length, ledPanelFile.offset);
				if(sdCardCopyState == FAT_SD_CARD_SPI_CUSTOM_COPY_OK)
					ledPanelFile.offset += length;
				else
					SetFileInfomationEmty(&ledPanelFile);
				
				// Notify task if queue render empty
				if(xSemaphoreTake(queueGdmaDescriptorsMutex, portMAX_DELAY) == pdTRUE){
					if(CheckQueueVectorGdmaDescriptorsNodeState() != QUEUE_VECTOR_DESCRIPTIORS_FULL)
						TaskNotify(&CoverntImageToVectorGdmadescriptorsNodeHandle);
					xSemaphoreGive(queueGdmaDescriptorsMutex);
				}
				
			}
		}
		taskYIELD();
	}
}

//-------------------------------------------------------------------------------/

void Setup(){
	fps = 60;
}

void TaskNotify(TaskHandle_t *task){
	if(eTaskGetState(*task) == eBlocked){
		xTaskNotifyGive(*task);
	}
}

void RequestNextGdmaDescriptorsNodeInQueue(){
	QueueVectorGdmaDescriptorsNodeState state = CheckQueueVectorGdmaDescriptorsNodeState();
	if(state != QUEUE_VECTOR_DESCRIPTIORS_EMPTY){
		RequestNextVectorGdmaDescriptorsNode(&ledPanelConfig);
	}
}


RandomImageRawNameState RandomImageRawName(char *path){
	DirentLinkerList list;
	GetListFileSdCardSPI(PATH_DIR_IMAGE_RAW, &list);
  	for(uint32_t i = 0; i < list.size; i++){
		  DirentNode *node = DirentLinkerListGetIndex(&list, i);
		  ESP_LOGI("list", "%s", node->name);
	}
	uint32_t size = list.size;
	if(size == 0){
		ESP_LOGE(RANDOM_IMAGE_RAW_NAME, RANDOM_IMAGE_RAW_FAIL_CAUSE_DIRECTORY_EMPTY_CONTENT);
		return RANDOM_IMAGE_RAW_NAME_FAIL_CAUSE_DIRECTORY_EMPTY;
	}
	uint32_t indexRandom = esp_random() % size;
	DirentNode *node = DirentLinkerListGetIndex(&list, indexRandom);
	memcpy(path, PATH_DIR_IMAGE_RAW, strlen(PATH_DIR_IMAGE_RAW));
	memcpy(path + strlen(PATH_DIR_IMAGE_RAW), node->name, strlen(node->name) + 1);
	DirentLinkerListDetelte(&list);
	return RANDOM_IMAGE_RAW_NAME_OK;
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
