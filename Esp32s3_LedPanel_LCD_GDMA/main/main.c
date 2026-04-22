#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "DirentLinkerListComponent.h"
#include "DmascriporsManager.h"
#include "Hub75ELut.h"
#include "LedPanelComponent.h"
#include "MessageComponent.h"
#include "QueueImageRaw.h"
#include "QueueMessageWifi.h"
#include "MessageComponent.h"
#include "TcpCustom.h"
#include "WifiCustomComponent.h"
#include "driver/spi_common.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi_types_generic.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "lwip/sockets.h"
#include "nvs_flash.h"
#include "os.h"
#include "portmacro.h"
#include "soc/gpio_num.h"
#include "FatSdCardSpiCustom.h"
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
#define WIFI_MESSAGE_QUEUE_BUFFER_SIZE												(HEADER_WIFI_MESSAGE + BYTE_IN_FRAME) + UINT_WIFI_MESSAFE_HEADER_MAX
#define UINT_WIFI_MESSAFE_HEADER_MAX												500
#define BYTE_IN_FRAME																(LEDPANEL_WIDTH * LEDPANEL_HEIGTH * HUB75E_LUT_COLOR)		
#define SD_CARD_FREQ																4000
#define MSG_WIFI_ID																	0x99
//-------------------------------------------------------------------------------/
#define QUEUE_SD_CARD_REQUEST_LENGTH												10
#define SECOND_UINT																	1000
#define DIRECTORY_NAME																"ImageRaw"
#define PATH_DIR_IMAGE_RAW															FAT_SD_CARD_SPI_CUSTOM_MOUSNT_PATH "/" DIRECTORY_NAME "/"
#define SIZE_NAME																	256
#define BUFFER_SIZE																	4096
#define FPS_DEFAULT																	24
#define PORT																		1111
#define WIFI_AP_SSID 																"ESP32_LED_PANEL_WIFI"
#define WIFI_AP_PASS 																"12345678"
#define WIFI_NETWORK_PART															192
#define WIFI_SUBNET_PART															168
#define WIFI_SUBNET																	10
#define WIFI_HOST_ID																11
#define WIFI_CHANNEL																1
#define WIFI_MAX_CONNECTION															4
#define KEEPALIVE_ENABLE															1
#define KEEPALIVE_IDLE             													15
#define KEEPALIVE_INTERVAL          												5
#define KEEPALIVE_COUNT             												3
#define TCP_RECEIVE_DATA_BLOCKING													0
#define TCP_SEND_DATA_BLOCKING														0
#define QUEUE_MESSAGE_WIFI_LENGTH													20
#define QUEUE_GDMA_DESCRIPTORS_SIZE													2
#define QUEUE_IMAGE_RAW_SIZE														100
#define GAMMA_SCALE																	2.2
#define RED_SCALE																	1
#define GREEN_SCALE																	0.8
#define BLUE_SCALE																	1
#define BLOCK_DATA_MAX																1024
#define NO_DELAY																	0
#define LIST_SIZE																	100
#define LIST_BUFFER_SIZE															255
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
#define TAG_TCP_SERVER_RECEIVE_TASK													"TAG_TCP_SERVER_RECEIVE_TASK"
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

typedef enum{
	LIST_FILE_IN_DIRECTORY_IMAGE_RAW_FOLDER = 0x33,
	NAME_FILE_IN_LIST						= 0x53,
	BLOCK_FILE								= 0x77,
	HEADER_CODE 							= 0x89,
	TOTAL_BLOCK_IN_IN_FILE					= 0x99,
	READ_FILE_IN_LIST						= 0x129,
	READ_FILE_SETUP_DONE					= 0x211,
	READ_FILE_ERROR							= 0x300,
	WRITE_BLOCK_FILE						= 0x310,
	WRITE_BLOCK_SETUP_DONE					= 0x333,
	WRITE_BLOCK_MSG							= 0x366,
	WRITE_FILE_ERROR						= 0x401,
	SD_CARD_USED 							= 0x410,
	SD_CARD_TOTAL 							= 0x415,
	BYTE_CONTENT							= 0x999
} CodeOpEnum;

typedef enum SdCardRequestType{
	SD_CARD_READ_FILE_REQUEST,
	SD_CARD_WRITE_FILE_REQUEST,
	SD_CARD_OPEN_FILE_REQUEST,
	SD_CARD_GET_LIST_FILE_REQUEST,
	SD_CARD_CLOSE_FILE_REQUEST,
	SD_CARD_GET_MEMORY_CARD_REQUEST
} SdCardRequestType;

//-------------------------------------------------------------------------------/

typedef struct{
	FILE *file;
	uint8_t *buffer;
	uint32_t length;
	FatSdCardSpiCustomCopyState *result;
} SdCardReadFileInfo;

typedef struct{
	FILE *file;
} SdCardCloseFileInfo;

typedef struct{
	FILE *file;
	uint8_t *buffer;
	uint32_t length;
	FatSdCardSpiCustomWriteState *result;
} SdCardWriteFileInfo;

typedef struct{
	FILE *file;
	char *path;
	char *type;
} SdCardOpenFileinfo;

typedef struct{
	char *path;
	DirentLinkerList *list;
	FatSdCardSpiCustomReadListFileState *result;
} SdCardGetListFileInfo;

typedef struct{
	uint64_t *free;
	uint64_t *used;
	uint64_t *total;
} SdCardGetMemoryInfo;

typedef struct{
	SdCardRequestType type;
	union{
		SdCardReadFileInfo readField;
		SdCardWriteFileInfo writeField;
		SdCardOpenFileinfo openField;
		SdCardGetListFileInfo getListFileField;
		SdCardCloseFileInfo closeField;
		SdCardGetMemoryInfo memoryField;
	};
	
	TaskHandle_t taskRequest;
} SdCardRequest;

typedef struct sdCardMemory{
	uint64_t total;
	uint64_t free;
	uint64_t used;
} SdCardMemory;


//-------------------------------------------------------------------------------//
QueueHandle_t queueSdCardRequest;

SdCardMemory sdMemoryInfo;
DirentLinkerList list;
SemaphoreHandle_t listMuxtex;

QueueWifiMessage queueMsgSend, queueMsgReceive;
QueueImageRaw ledPanelImageRaw;
const char pathImageRawFolder[] = FAT_SD_CARD_SPI_CUSTOM_MOUSNT_PATH "/" PATH_DIR_IMAGE_RAW;
LedPanelConfig ledPanelConfig;

FatSdCardSpiCustomConfig sdCardConfig;
uint32_t fps;
uint32_t timeLedPanelUpdate;
int tcpServerSocket;
//-------------------------------------------------------------------------------/
void SetSdCardGetMemoryRequest(SdCardRequest *request, uint64_t *free, uint64_t *used, uint64_t *total, TaskHandle_t task);
void ReadSdCardTotalRequest(int *client);
void ReadSdCardUsedRequest(int *client);
void SetSdCardOpenFileRequest(SdCardRequest *request, char *path, char* mode, TaskHandle_t task);
void SetSdCardCloseFileRequest(SdCardRequest *request, FILE *file, TaskHandle_t task);
void SetSdCardReadFileRequest(SdCardRequest *request, FILE *file, uint8_t *buffer, uint32_t length, FatSdCardSpiCustomCopyState *state, TaskHandle_t task);
void ReadFileRequest(int *client, uint8_t *name, uint32_t size, TaskHandle_t task);
void TcpServerConfig();
void WifiConfig();
RandomImageRawNameState RandomImageRawName(char *path, DirentLinkerList *list);
void LedPanelConfigInit(LedPanelConfig *config);
void FatSdCardSpiCustomConfigInit(FatSdCardSpiCustomConfig *config, SdCardMemory *info);
void LedPanelTask(void *pvParameters);
void LedPanelImageRawReadTask(void *pvParameters);
void CoverntImageToVectorGdmadescriptorsNodeTask(void *pvParameters);
void SdCardSpiTask(void *pvParameters);
void TcpServerReceiveTask(void *pvParameter);
void TcpServerSendTask(void* pvParameter);
void SemaphoreInit();
void CreateTask();
void SetupParameter();
void HandleWifiMessageTask(void* pvParameter);
void ReadListFileInImageRawFolder(int *client, DirentLinkerList *list);
uint32_t WriteFileSetupRequest(int *client, FILE **file, char* name, uint32_t size, TaskHandle_t task);
BaseType_t WriteFileRequest(int *client, FILE * file, uint8_t *buffer, uint32_t code, uint32_t size, uint32_t *index, uint32_t countMsg, TaskHandle_t task);
void SetSdCardWriteFileRequest(SdCardRequest *request, FILE *file, uint8_t *buffer, uint32_t size, FatSdCardSpiCustomWriteState *result, TaskHandle_t task);
void FindAndSwapNameFile(char *name, DirentLinkerList *list);
//-------------------------------------------------------------------------------/

void app_main(void)
{
	SetupParameter();
	ESP_LOGI(TAG_APP_MAIN, APP_MAIN_INIT);
	WifiConfig();
	TcpServerConfig();
	FatSdCardSpiCustomConfigInit(&sdCardConfig, &sdMemoryInfo);
	LedPanelConfigInit(&ledPanelConfig);
	SemaphoreInit();
	CreateTask();
	
	size_t free_ram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

	ESP_LOGE("RAM", "Free internal RAM: %d bytes (~%d KB)", 
         free_ram, free_ram / 1024);
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);

	ESP_LOGE("RAM", "Largest block: %d bytes (~%d KB)", 
         largest, largest / 1024);
         
}

//-------------------------------------------------------------------------------/
void SetSdCardGetMemoryRequest(SdCardRequest *request, uint64_t *free, uint64_t *used, uint64_t *total, TaskHandle_t task){
	request->type = SD_CARD_GET_MEMORY_CARD_REQUEST;
	request->memoryField.free = free;
	request->memoryField.total = total;
	request->memoryField.used = used;
	request->taskRequest = task;
	xQueueSend(queueSdCardRequest, &request, portMAX_DELAY);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

void HandleWifiMessageTask(void* pvParameter){
	WifiMessage *msg;
	HeaderFieldMessage info;
	FILE *file;
	TaskHandle_t task = xTaskGetCurrentTaskHandle();
	uint32_t code;
	uint32_t countMsg;
	uint32_t indexMsg;
	while(1){
		if(GetQueueWifiMessageMsgReady(&queueMsgReceive, &msg, QUEUE_MESSAGE_WIFI_BLOCK) == pdTRUE){
			GetFieldInMessage(msg->buffer, &info);
			switch(info.code){
				case LIST_FILE_IN_DIRECTORY_IMAGE_RAW_FOLDER:
					if(xSemaphoreTake(listMuxtex, portMAX_DELAY) == pdTRUE){
						ReadListFileInImageRawFolder(&msg->client, &list);
						xSemaphoreGive(listMuxtex);
					}		
					break;
				case READ_FILE_IN_LIST:
					ReadFileRequest(&msg->client, msg->buffer + CONTENT_FIELD_MESSAGE_START, info.length, task);
					break;
				case SD_CARD_USED: 
					ReadSdCardUsedRequest(&msg->client);
					break;
				case SD_CARD_TOTAL:
					ReadSdCardTotalRequest(&msg->client);
					break;
				case WRITE_BLOCK_FILE: {
					uint32_t sizeName = info.length;
					GetFieldInMessage( msg->buffer + CONTENT_FIELD_MESSAGE_START + sizeName, &info);
					if(info.code == TOTAL_BLOCK_IN_IN_FILE){
						indexMsg = 0;
						countMsg = GetUlongInMessage(msg->buffer + CONTENT_FIELD_MESSAGE_START + sizeName);
						code = WriteFileSetupRequest(&msg->client, &file, (char*)msg->buffer + CONTENT_FIELD_MESSAGE_START, sizeName, task);
					}
					ESP_LOGI("ptr", "ptr=%p", file);
					break;
				}
				
				case WRITE_BLOCK_MSG: {
					uint32_t codeMsg = GetUlongInMessage(msg->buffer);
					if(codeMsg != code)
						break;
					GetFieldInMessage(msg->buffer + HEADER_WIFI_MESSAGE, &info);
					if(info.code == BYTE_CONTENT){
						ESP_LOGI("HandleWifiMessageTask", "write request");
						WriteFileRequest(&msg->client,
										file,
										msg->buffer + HEADER_WIFI_MESSAGE + CONTENT_FIELD_MESSAGE_START,
										code,
										info.length,
										&indexMsg,
										countMsg,
										task
						);
					}
					break;
				}
				
			}
			PushQueueWifiMessagePointerFree(&queueMsgReceive, msg, QUEUE_MESSAGE_WIFI_BLOCK);
		}
	}
}

//-------------------------------------------------------------------------------/
void FindAndSwapNameFile(char *name, DirentLinkerList *list){
	char temp[SIZE_NAME];
	uint32_t index = 0;
	DirentNode *one = NULL;
	DirentNode *find = NULL;
	for(;index < list->size; index++){
		one = DirentLinkerListGetIndex(list, index);
		if(strcmp((char *)one->buffer, name) == 0){
			memcpy(temp, one->buffer, strlen((char*)one->buffer));
			find = one;
			break;
		}
	}
	DirentNode *two = DirentLinkerListGetIndex(list, list->size - 1);
	if(find != NULL){
		memcpy(one->buffer, two->buffer, strlen((char*)two->buffer) + 1);
		memcpy(two->buffer, temp, strlen(temp) + 1);
		list->size--;
	} else {
		two = two->next;
		memcpy(two->buffer, name, strlen(name));
		two->buffer[strlen(name)] = '\0';
	}
}

void SetSdCardWriteFileRequest(SdCardRequest *request, FILE *file, uint8_t *buffer, uint32_t size, FatSdCardSpiCustomWriteState *result, TaskHandle_t task){
	request->type = SD_CARD_WRITE_FILE_REQUEST;
	request->writeField.file = file;
	request->writeField.buffer = buffer;
	request->writeField.length = size;
	request->writeField.result = result;
	request->taskRequest = task;
	xQueueSend(queueSdCardRequest, &request, portMAX_DELAY);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

BaseType_t WriteFileRequest(int *client, FILE * file, uint8_t *buffer, uint32_t code, uint32_t size, uint32_t *index, uint32_t countMsg, TaskHandle_t task){
	SdCardRequest request;
	FatSdCardSpiCustomWriteState state;
	ESP_LOGI("WriteFileSetupRequest", "open %p", file);
	SetSdCardWriteFileRequest(&request, file, buffer, size, &state, task);
	if((*index != countMsg - 1) && (state == FAT_SD_CARD_SPI_CUSTOM_WRITE_OK)){
		(*index)++;
		return pdFALSE;
	}
	WifiMessage *msg; 	
	GetQueueWifiMessagePointerFree(&queueMsgSend, &msg, QUEUE_MESSAGE_WIFI_BLOCK);
	msg->client = *client;
	msg->length = 0;
	switch(state){
	case FAT_SD_CARD_SPI_CUSTOM_WRITE_OK:
		msg->length += AddUlongToMessage(msg->buffer + msg->length, TOTAL_BLOCK_IN_IN_FILE, code);
		SetSdCardCloseFileRequest(&request, file, task);
		SetSdCardGetMemoryRequest(&request, &sdMemoryInfo.free, &sdMemoryInfo.used, &sdMemoryInfo.total, task);
		msg->length += AddUlongToMessage(msg->buffer + msg->length, SD_CARD_USED, sdMemoryInfo.used);
		
		if(xSemaphoreTake(listMuxtex, portMAX_DELAY) == pdTRUE){
			list.size++;
			xSemaphoreGive(listMuxtex);
		}
		
		break;
	case FAT_SD_CARD_SPI_CUSTOM_WRITE_FAIL_CAUSE_WRITE_FILE_ERROR:
	case FAT_SD_CARD_SPI_CUSTOM_WRITE_FAIL_CAUSE_PARTIAL_WRITE:
		SetSdCardCloseFileRequest(&request, file, task);
	case FAT_SD_CARD_SPI_CUSTOM_WRITE_FAIL_CAUSE_OPEN_FILE_FAIL:
		msg->length += AddUlongToMessage(msg->buffer + msg->length, WRITE_FILE_ERROR, state);
		break;	
	default:
		break;
	}
	PushQueueWifiMessageMsgReady(&queueMsgSend, msg, QUEUE_MESSAGE_WIFI_BLOCK);
	return pdTRUE;
}

uint32_t WriteFileSetupRequest(int *client, FILE **file, char* name, uint32_t size, TaskHandle_t task){
	char path[SIZE_NAME];
	char mode[] = "w+";
	memcpy(path, PATH_DIR_IMAGE_RAW, strlen(PATH_DIR_IMAGE_RAW));
	if(xSemaphoreTake(listMuxtex, portMAX_DELAY) == pdTRUE){
		FindAndSwapNameFile(name, &list);
		xSemaphoreGive(listMuxtex);
	}
	memcpy(path + strlen(PATH_DIR_IMAGE_RAW), name, size);
	path[strlen(PATH_DIR_IMAGE_RAW) + size] = '\0'; 
	SdCardRequest request;
	SetSdCardOpenFileRequest(&request, path, mode, task);
	*file = request.openField.file; 
	if(request.openField.file == NULL){
		ESP_LOGE("WriteFileSetupRequest", "open fail");
		return 0;
	} else
		ESP_LOGI("WriteFileSetupRequest", "open %p", request.openField.file);
	//uint32_t codeMsg =  esp_random();
	uint32_t codeMsg = 10;
	WifiMessage *msg;
	GetQueueWifiMessagePointerFree(&queueMsgSend, &msg, QUEUE_MESSAGE_WIFI_BLOCK);
	msg->client = *client;
	msg->length = AddUlongToMessage(msg->buffer, WRITE_BLOCK_SETUP_DONE, codeMsg);
	PushQueueWifiMessageMsgReady(&queueMsgSend, msg, QUEUE_MESSAGE_WIFI_BLOCK);
	ESP_LOGI("WriteFileSetupRequest", "%s", path);
	ESP_LOGI("WriteFileSetupRequest", "%llu", (unsigned long long) codeMsg);
	return codeMsg;
}

void ReadSdCardTotalRequest(int *client){
	WifiMessage *msg;
	GetQueueWifiMessagePointerFree(&queueMsgSend, &msg, QUEUE_MESSAGE_WIFI_BLOCK);
	msg->client = *client;
	msg->length = AddUlongToMessage(msg->buffer, SD_CARD_TOTAL, sdMemoryInfo.total);
	PushQueueWifiMessageMsgReady(&queueMsgSend, msg, QUEUE_MESSAGE_WIFI_BLOCK);
}

void ReadSdCardUsedRequest(int *client){
	WifiMessage *msg;
	GetQueueWifiMessagePointerFree(&queueMsgSend, &msg, QUEUE_MESSAGE_WIFI_BLOCK);
	msg->client = *client;
	msg->length = AddUlongToMessage(msg->buffer, SD_CARD_USED, sdMemoryInfo.used);
	PushQueueWifiMessageMsgReady(&queueMsgSend, msg, QUEUE_MESSAGE_WIFI_BLOCK);
}

void ReadFileRequest(int *client, uint8_t *name, uint32_t size, TaskHandle_t task){
	char path[SIZE_NAME];
	char mode[] = "rb";
	memcpy(path, PATH_DIR_IMAGE_RAW, strlen(PATH_DIR_IMAGE_RAW));
	memcpy(path + strlen(PATH_DIR_IMAGE_RAW), name, size);
	path[strlen(PATH_DIR_IMAGE_RAW) + size] = '\0'; 
	uint32_t code = esp_random();
	uint32_t block = 0;
	WifiMessage *msg;
	if(GetQueueWifiMessagePointerFree(&queueMsgSend, &msg, QUEUE_MESSAGE_WIFI_BLOCK) == pdTRUE){
		uint32_t size = AddUlongToMessage(msg->buffer, READ_FILE_SETUP_DONE, code);
		msg->length = size;
		msg->client = *client;
		PushQueueWifiMessageMsgReady(&queueMsgSend, msg, QUEUE_MESSAGE_WIFI_BLOCK);
	}
	SdCardRequest request;
	SetSdCardOpenFileRequest(&request, path, mode, task);
	if(request.openField.file == NULL)
		return;
	FatSdCardSpiCustomCopyState state = FAT_SD_CARD_SPI_CUSTOM_COPY_OK;
	FILE *file = request.openField.file;
	while(state == FAT_SD_CARD_SPI_CUSTOM_COPY_OK){
		GetQueueWifiMessagePointerFree(&queueMsgSend, &msg, QUEUE_MESSAGE_WIFI_BLOCK);
		msg->client = *client;
		msg->length = HEADER_WIFI_MESSAGE + CONTENT_FIELD_MESSAGE_START;
		SetSdCardReadFileRequest(&request, file, msg->buffer + msg->length, BYTE_IN_FRAME, &state, task);
		switch (state){
			case FAT_SD_CARD_SPI_CUSTOM_COPY_OK:
				msg->length += BYTE_IN_FRAME;
				block++;
				AddUlongToMessage(msg->buffer, BLOCK_FILE, block);
				AddHeaderFieldMessage(msg->buffer + HEADER_WIFI_MESSAGE, BYTE_CONTENT, BYTE_IN_FRAME);
				ESP_LOGI("msg block", "msg block = %llu, %llu", (unsigned long long) block, (unsigned long long) msg->length);
				break;
			case FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_READ_FILE_ERROR:
			case FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_OPEN_FILE_FAIL:
				AddUlongToMessage(msg->buffer, READ_FILE_ERROR, block);
				msg->length = HEADER_WIFI_MESSAGE;
				ESP_LOGI("msg error", "msg error, %llu byte", (unsigned long long) msg->length);
				SetSdCardCloseFileRequest(&request, file, task);
				break;
			case FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_FILE_END:
				AddUlongToMessage(msg->buffer, TOTAL_BLOCK_IN_IN_FILE, block);
				msg->length = HEADER_WIFI_MESSAGE;
				ESP_LOGI("msg end", "msg end = %llu", (unsigned long long) block);
				SetSdCardCloseFileRequest(&request, file, task);
				break;
		}
		PushQueueWifiMessageMsgReady(&queueMsgSend, msg, QUEUE_MESSAGE_WIFI_BLOCK);
	}
	ESP_LOGI("msg count", "msg count = %llu", (unsigned long long) block);
}

void ReadListFileInImageRawFolder(int *client, DirentLinkerList *list){
	WifiMessage *msg;
	if(GetQueueWifiMessagePointerFree(&queueMsgSend, &msg, QUEUE_MESSAGE_WIFI_BLOCK) == pdTRUE){
		msg->client = *client;
		msg->length = AddUlongToMessage((uint8_t*)msg->buffer, LIST_FILE_IN_DIRECTORY_IMAGE_RAW_FOLDER, list->size);
		if(list->size != 0){
			msg->length += AddListToMessage((uint8_t*)msg->buffer + msg->length, NAME_FILE_IN_LIST, list);
		}
		PushQueueWifiMessageMsgReady(&queueMsgSend, msg, QUEUE_MESSAGE_WIFI_BLOCK);
	}
}

void SetupParameter(){
	queueSdCardRequest = xQueueCreate(QUEUE_SD_CARD_REQUEST_LENGTH, sizeof(SdCardRequest*));
	fps = FPS_DEFAULT;
}

void SemaphoreInit(){
	listMuxtex = xSemaphoreCreateMutex();
}

//-------------------------------------------------------------------------------------------------------------------/

void TcpServerConfig(){
	TcpCustomInit(&tcpServerSocket, PORT);
	TcpCustomSetId(HEADER_CODE);
	QueueWifiMessageInit(&queueMsgReceive, QUEUE_MESSAGE_WIFI_LENGTH, WIFI_MESSAGE_QUEUE_BUFFER_SIZE);
	QueueWifiMessageInit(&queueMsgSend, QUEUE_MESSAGE_WIFI_LENGTH, WIFI_MESSAGE_QUEUE_BUFFER_SIZE);
}

void WifiConfig(){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    wifi_config_t config = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .ssid_len = strlen(WIFI_AP_SSID),
            .channel = 1,
            .password = WIFI_AP_PASS,
            .max_connection = WIFI_MAX_CONNECTION,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .ssid_hidden = 0,
            .bss_max_idle_cfg = {
                .period = WIFI_AP_DEFAULT_MAX_IDLE_PERIOD,
                .protected_keep_alive = 1,
            }
        }
    };
    WifiCustomInit(&config, WIFI_NETWORK_PART, WIFI_SUBNET_PART, WIFI_SUBNET, WIFI_HOST_ID);
}


RandomImageRawNameState RandomImageRawName(char *path, DirentLinkerList *list){
	if(list->size == 0)
		return RANDOM_IMAGE_RAW_NAME_FAIL_CAUSE_DIRECTORY_EMPTY;
	uint32_t indexRandom = esp_random() % list->size;
	DirentNode *node = DirentLinkerListGetIndex(list, indexRandom);
	memcpy(path, PATH_DIR_IMAGE_RAW, strlen(PATH_DIR_IMAGE_RAW));
	memcpy(path + strlen(PATH_DIR_IMAGE_RAW), node->buffer, strlen((char*)node->buffer) + 1);
	return RANDOM_IMAGE_RAW_NAME_OK;
}

void FatSdCardSpiCustomConfigInit(FatSdCardSpiCustomConfig *config, SdCardMemory *info){
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
   	config->host.max_freq_khz = SD_CARD_FREQ;
    FatSdCardSpiCustomInit(config);
    GetSdCardInfo(&info->total, &info->free, &info->used);
}

void LedPanelConfigInit(LedPanelConfig *config){
	config->style.width = LEDPANEL_WIDTH;
	config->style.heigth = LEDPANEL_HEIGTH;
	config->style.scan = LEDPANEL_SCAN_2_PART;
	config->style.color = LEDPANEL_RGB888;
	config->style.gamma = GAMMA_SCALE;
	config->style.redScale = RED_SCALE;
	config->style.greenScale = GREEN_SCALE;
	config->style.blueScale= BLUE_SCALE;
	
	gpio_num_t ledPanelPin[] = {
		LEDPANEL_PIN_R1, LEDPANEL_PIN_G1, LEDPANEL_PIN_B1,
		LEDPANEL_PIN_R2, LEDPANEL_PIN_G2, LEDPANEL_PIN_B2,
		LEDPANEL_PIN_A, LEDPANEL_PIN_B, LEDPANEL_PIN_C, LEDPANEL_PIN_D, LEDPANEL_PIN_E,
		 LEDPANEL_PIN_LATCH, LEDPANEL_PIN_OE, LEDPANEL_PIN_CLK
	};
	
	LedPanelInit(config, ledPanelPin, QUEUE_GDMA_DESCRIPTORS_SIZE);
	QueueImageRawInit(&ledPanelImageRaw, QUEUE_IMAGE_RAW_SIZE, LEDPANEL_WIDTH, LEDPANEL_HEIGTH);
	ImageRaw *raw;
	if(GetQueueImageRawFree(&ledPanelImageRaw, &raw, IMAGE_RAW_BLOCK) == pdTRUE){
		for(uint32_t i = 0; i < BYTE_IN_FRAME; i++){
			*(raw->buffer + i) = 0xff;
		}
	}
	VectorGdmaDescriptorsNode *gdma = NULL;
	GetDmaDescriptorFree(&gdma, DMA_MANAGER_TIME_BLOCK);
    LedPanelConvertFrameData(gdma, 
                         &ledPanelConfig.style, 
                         &ledPanelImageRaw.config, 
                         raw);
	PushQueueImageRawFree(&ledPanelImageRaw, raw, IMAGE_RAW_BLOCK);
	LedPanelStartTransmit(&ledPanelConfig, gdma);
	DirentLinkerListInit(&list, LIST_SIZE, LIST_BUFFER_SIZE);
	GetListFileSdCardSPI(PATH_DIR_IMAGE_RAW, &list);
}


void LedPanelTask(void *pvParameters){
	VectorGdmaDescriptorsNode *gdma = NULL;
	int64_t start, end;

	while (1) {
		start = esp_timer_get_time();
		GetDmaDescriptorReady(&gdma, DMA_MANAGER_TIME_BLOCK);
		LedPanelRestart(&ledPanelConfig, gdma);
		end = esp_timer_get_time();
		int64_t timeSetup = (end - start) / SECOND_UINT;
		//ESP_LOGE("ledPanel", "Time: %lld ms", timeSetup);
		if(SECOND_UINT/fps > timeSetup){
			ESP_LOGI("Delay", "%llu", SECOND_UINT/fps - timeSetup);
			vTaskDelay(pdMS_TO_TICKS(SECOND_UINT/fps - timeSetup));
		}
	}
}


void CoverntImageToVectorGdmadescriptorsNodeTask(void *pvParameters){
	VectorGdmaDescriptorsNode *gdma = NULL;
	ImageRaw *raw = NULL;
	while (1) {
		if (raw == NULL) {
	        GetQueueImageRawReady(&ledPanelImageRaw, &raw, IMAGE_RAW_BLOCK);
	    }
	    if (gdma == NULL) {
	        GetDmaDescriptorFree(&gdma, DMA_MANAGER_TIME_BLOCK);
	    }

		if (gdma != NULL && raw != NULL) {
		
		    LedPanelConvertFrameData(gdma, 
		                             &ledPanelConfig.style, 
		                             &ledPanelImageRaw.config, 
		                             raw);
		
		    PushDmaDescriptorReady(gdma, DMA_MANAGER_TIME_BLOCK);
		    PushQueueImageRawFree(&ledPanelImageRaw, raw, IMAGE_RAW_BLOCK);
		
		    gdma = NULL;
		    raw = NULL;
		}
	}
}

void TcpServerSendTask(void* pvParameter){
	WifiMessage *msg;
	while(1){
		if(GetQueueWifiMessageMsgReady(&queueMsgSend, &msg, QUEUE_MESSAGE_WIFI_BLOCK) == pdTRUE){
			ESP_LOGE("Test", "send msg");
			TcpCustomSendMessage(msg);
			PushQueueWifiMessagePointerFree(&queueMsgSend, msg, QUEUE_MESSAGE_WIFI_BLOCK);
		}
	}
}

void TcpServerReceiveTask(void *pvParameter){
    struct sockaddr_storage sourceAddr;
    socklen_t addrLen = sizeof(sourceAddr);
    int clientSocket;
    int keepAlive = KEEPALIVE_ENABLE;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    WifiMessage *msg = NULL;
    TcpClientReceiveStateEnum state;
	while(1){
		TcpCustomAccept(&tcpServerSocket, &sourceAddr, &addrLen, &clientSocket);
		if(clientSocket < 0){
            ESP_LOGE(TAG_TCP_SERVER_RECEIVE_TASK, "Unable to accept connection: errno %d", errno);
            break;
		}
		TcpServerSetKeepAlive(&clientSocket, &keepAlive, &keepIdle, &keepInterval, &keepCount);
		
		while(1){
			if(GetQueueWifiMessagePointerFree(&queueMsgReceive, &msg, QUEUE_MESSAGE_WIFI_BLOCK) == pdTRUE){
				ESP_LOGE("Test", "wait msg");
			    state = TcpCustomReceiveMsg(clientSocket, msg);
			    if (state == TCP_SERVER_RECEIVE_MSG_OK) {
					msg->client = clientSocket;
			        PushQueueWifiMessageMsgReady(&queueMsgReceive, msg, QUEUE_MESSAGE_WIFI_BLOCK);
			        ESP_LOGE("Test", "receive msg");
			        continue;
			    }
			    PushQueueWifiMessagePointerFree(&queueMsgReceive, msg, QUEUE_MESSAGE_WIFI_BLOCK);
			    if (state == TCP_CLIENT_DISCONNECT || state == TCP_CLIENT_TIMEOUT){
			    	ESP_LOGE("Test", "msg invalid");
			        break;
			     }
				
			}
		}
		close(clientSocket);
	}
}

void LedPanelImageRawReadTask(void *pvParameters){
	TaskHandle_t task = xTaskGetCurrentTaskHandle();
	SdCardRequest request;
	ImageRaw *raw;
	FatSdCardSpiCustomCopyState state;
	char path[SIZE_NAME];
	char mode[] = "rb";
	FILE *file = NULL; 
	while(1){
		if(GetQueueImageRawFree(&ledPanelImageRaw, &raw, IMAGE_RAW_BLOCK) == pdTRUE){
			if(file == NULL){
				if(xSemaphoreTake(listMuxtex, portMAX_DELAY) == pdTRUE){
					RandomImageRawName(path, &list);
					xSemaphoreGive(listMuxtex);
				}
				SetSdCardOpenFileRequest(&request, path, mode, task);
				file = request.openField.file;
				if(file == NULL){
					PushQueueImageRawFree(&ledPanelImageRaw, raw, IMAGE_RAW_BLOCK);
					vTaskDelete(NULL);
				}
					
			}
			SetSdCardReadFileRequest(&request, file, raw->buffer, BYTE_IN_FRAME, &state, task);
			switch (state) {
				case FAT_SD_CARD_SPI_CUSTOM_COPY_OK: 
					PushQueueImageRawReady(&ledPanelImageRaw, raw, IMAGE_RAW_BLOCK);
					raw = NULL;
					break;
				case FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_OPEN_FILE_FAIL:
					vTaskDelete(NULL);
					break;
				case FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_READ_FILE_ERROR:
					vTaskDelete(NULL);
					break;
				case FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_FILE_END:
					if(file != NULL){
						SetSdCardCloseFileRequest(&request, file, task);
						file = NULL;
					}
					PushQueueImageRawFree(&ledPanelImageRaw, raw, IMAGE_RAW_BLOCK);
					break;
			}
		}
	}
}

void SdCardSpiTask(void *pvParameters){
	SdCardRequest *request;
	TaskHandle_t task;
	while (1) {
		if(xQueueReceive(queueSdCardRequest, &request, portMAX_DELAY) == pdTRUE){
			switch (request->type) {
				case SD_CARD_OPEN_FILE_REQUEST:
					request->openField.file = fopen(request->openField.path, request->openField.type);
					ESP_LOGI("test", "open %s %s", request->openField.path, request->openField.type);
					break;
				case SD_CARD_READ_FILE_REQUEST:
					*request->readField.result = CopySdCardSpiFile(request->readField.file, request->readField.buffer, request->readField.length);
					break;
				case SD_CARD_WRITE_FILE_REQUEST:
					*request->writeField.result = WriteSdCardSpiFileOptimized(request->writeField.file, request->writeField.buffer, request->writeField.length);
					break;
				case SD_CARD_GET_LIST_FILE_REQUEST:
					*request->getListFileField.result = GetListFileSdCardSPI(request->getListFileField.path, request->getListFileField.list);
					break;
				case SD_CARD_CLOSE_FILE_REQUEST:
					fclose(request->closeField.file);
					break;
				case SD_CARD_GET_MEMORY_CARD_REQUEST:
					GetSdCardInfo(request->memoryField.total, request->memoryField.free, request->memoryField.used);
			}
			task = request->taskRequest;
			request = NULL;
			xTaskNotifyGive(task);
		}
	}
}

void SetSdCardReadFileRequest(SdCardRequest *request, FILE *file, uint8_t *buffer, uint32_t length, FatSdCardSpiCustomCopyState *state, TaskHandle_t task){
	request->type = SD_CARD_READ_FILE_REQUEST;
	request->readField.file = file;
	request->readField.result = state;
	request->readField.buffer = buffer;
	request->readField.length = BYTE_IN_FRAME;
	request->taskRequest = task;
	xQueueSend(queueSdCardRequest, &request, portMAX_DELAY);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

void SetSdCardCloseFileRequest(SdCardRequest *request, FILE *file, TaskHandle_t task){
	request->type = SD_CARD_CLOSE_FILE_REQUEST;
	request->closeField.file = file;
	request->taskRequest = task;
	xQueueSend(queueSdCardRequest, &request, portMAX_DELAY);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

void SetSdCardOpenFileRequest(SdCardRequest *request, char *path, char* mode, TaskHandle_t task){
	request->type = SD_CARD_OPEN_FILE_REQUEST;
	request->openField.path = path;
	request->openField.type = mode;
	request->taskRequest = task;
	xQueueSend(queueSdCardRequest, &request, portMAX_DELAY);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}


void CreateTask(){
	xTaskCreatePinnedToCore(
        LedPanelImageRawReadTask,
        "LedPanelImageRawReadTask",
        3072,
        NULL,
        FREE_RTOS_PRIORITY_LOW,
        NULL,
        CORE_1
    );
	xTaskCreatePinnedToCore(
        LedPanelTask,
        "LedPanelTask",
        3072,
        NULL,
        FREE_RTOS_PRIORITY_REAL_TIME,
        NULL,
        CORE_1
    );
    xTaskCreatePinnedToCore(
        CoverntImageToVectorGdmadescriptorsNodeTask,
        "CoverntImageToVectorGdmadescriptorsNodeTask",
        3072,
        NULL,
        FREE_RTOS_PRIORITY_LOW,
        NULL,
        CORE_0
    );
    xTaskCreatePinnedToCore(
        SdCardSpiTask,
        "SdCardSpiTask",
        3072,
        NULL,
        FREE_RTOS_PRIORITY_MEDIUM,
        NULL,
        CORE_1
    );
	xTaskCreatePinnedToCore(
        TcpServerReceiveTask,
        "TcpServerReceiveTask",
        3072,
        NULL,
        FREE_RTOS_PRIORITY_REAL_TIME,
        NULL,
        CORE_0
    );
    xTaskCreatePinnedToCore(
        TcpServerSendTask,
        "TcpServerSendTask",
        3072,
        NULL,
        FREE_RTOS_PRIORITY_HIGH,
        NULL,
        CORE_0
    );
    xTaskCreatePinnedToCore(
        HandleWifiMessageTask,
        "HandleWifiMessageTask",
        3072,
        NULL,
        FREE_RTOS_PRIORITY_MEDIUM,
        NULL,
        CORE_1
    );
    
}