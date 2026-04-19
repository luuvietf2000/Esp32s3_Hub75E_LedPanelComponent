#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "DmascriporsManager.h"
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
#define SD_CARD_FREQ																SDMMC_FREQ_HIGHSPEED
#define MSG_WIFI_ID																	0x99
//-------------------------------------------------------------------------------/
#define SECOND_UINT																	1000
#define DIRECTORY_NAME																"ImageRaw"
#define PATH_DIR_IMAGE_RAW															FAT_SD_CARD_SPI_CUSTOM_MOUSNT_PATH "/" DIRECTORY_NAME "/"
#define SIZE_NAME																	256
#define BUFFER_SIZE																	4096
#define FPS_DEFAULT																	4
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
	SD_CARD_USED 							= 0x410
} CodeOpEnum;

typedef enum{
	READ_LIST_FILE_IN_DIRECTORY_STATE,
	READ_FILE_STATE,
	WRITE_FILE_STATE,
	NONE_FILE_STATE
} FileCommunicationState;

typedef enum{
	CLIENT_REQUEST,
	DONE_CLIENT_REQUEST
} ClientRequestState;

//-------------------------------------------------------------------------------/

typedef struct ClientRequestInformation{
	FileCommunicationState fileState;
	ClientRequestState clientState;
	int mSocket;
	FILE *file;
	uint32_t codeMsg;
	uint32_t block;
	uint32_t countMsg;
	uint32_t msgWrite;
	FileInfomation f;
} ClientRequestInformation;

typedef struct sdCardMemory{
	uint64_t total;
	uint64_t free;
	uint64_t used;
} SdCardMemory;

//-------------------------------------------------------------------------------//
SdCardMemory sdMemoryInfo;
SemaphoreHandle_t sdMemoryInfoMutex;
DirentLinkerList list;
SemaphoreHandle_t listMuxtex;
QueueWifiMessage queueMsgSend, queueMsgReceive;
QueueImageRaw ledPanelImageRaw;
QueueImageRaw wifiImageRaw;
const char pathImageRawFolder[] = FAT_SD_CARD_SPI_CUSTOM_MOUSNT_PATH "/" PATH_DIR_IMAGE_RAW;
LedPanelConfig ledPanelConfig;

FatSdCardSpiCustomConfig sdCardConfig;
uint32_t fps;
uint32_t timeLedPanelUpdate;
int tcpServerSocket;
ClientRequestInformation mClientRequestInformation;
SemaphoreHandle_t clientRequestMutex;
//-------------------------------------------------------------------------------/
void WriteFileInFolder(ClientRequestInformation *clientRequest);
void WriteFileRequest(ClientRequestInformation *clientRequest, char *nameFile, uint32_t size, uint32_t countMsg);
void HandleClientRequestSdCard(ClientRequestInformation *clientRequest);
void ReadFileRequest(ClientRequestInformation *clientRequest, uint8_t *nextDataPacket, uint32_t size);
void TcpServerConfig();
void WifiConfig();
RandomImageRawNameState RandomImageRawName(char *path, DirentLinkerList *list);
void LedPanelConfigInit(LedPanelConfig *config);
void FatSdCardSpiCustomConfigInit(FatSdCardSpiCustomConfig *config, SdCardMemory *info);
void LedPanelTask(void *pvParameters);
void CoverntImageToVectorGdmadescriptorsNodeTask(void *pvParameters);
void SdCardSpiTask(void *pvParameters);
void TcpServerReceiveTask(void *pvParameter);
void TcpServerSendTask(void* pvParameter);
void SemaphoreInit();
void CreateTask();
void SetupParameter();
void HandleWifiMessageTask(void* pvParameter);

void HandleMessage(WifiMessage *message, ClientRequestInformation *clientRequest);
void ReadListFileRequest(ClientRequestInformation *clientRequest);
void ReadListFileInImageRawFolder(ClientRequestInformation *clientRequest, DirentLinkerList *list);
void ReadListFileRequest(ClientRequestInformation *clientRequest);
void SetDoneHandleRequestClineInfomation(ClientRequestInformation *clientRequest);
void ReadFileInFolder(ClientRequestInformation *clientRequest);
void RequestReadFileAndPushInQueueImageRaw(FILE *file, uint32_t size, char *path, DirentLinkerList *list);
void PushBlockInQueueBlock(ClientRequestInformation *clientRequest, uint8_t *data, uint32_t indexMsg);
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

void HandleWifiMessageTask(void* pvParameter){
	WifiMessage *msg;
	while(1){
		if(GetQueueWifiMessageMsgReady(&queueMsgReceive, &msg, QUEUE_MESSAGE_WIFI_BLOCK) == pdTRUE){
			HandleMessage(msg, &mClientRequestInformation);
			PushQueueWifiMessagePointerFree(&queueMsgReceive, msg, QUEUE_MESSAGE_WIFI_BLOCK);
		}
	}
}


void SdCardSpiTask(void *pvParameters){
	FasrSdCardSpiCustomReadListFileState listState= GetListFileSdCardSPI(PATH_DIR_IMAGE_RAW, &list);
	if(FAT_SD_CARD_SPI_CUSTOM_READ_LIST_FILE_OK != listState)
		vTaskDelete(NULL);
	char *ledPanelPath = heap_caps_malloc(sizeof(char) * LIST_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
	RandomImageRawNameState randomState = RandomImageRawName(ledPanelPath, &list);
	FILE *ledPanelFile = fopen(ledPanelPath, "rb");
	while (1) {
		//int64_t start = esp_timer_get_time();
		if(randomState == RANDOM_IMAGE_RAW_NAME_OK)
			RequestReadFileAndPushInQueueImageRaw(ledPanelFile, BYTE_IN_FRAME, ledPanelPath, &list);
		//int64_t end = esp_timer_get_time();
		//ESP_LOGE("Read card", "Read time: %lld us", end - start);
		//HandleClientRequestSdCard(&mClientRequestInformation);
		
		//vTaskDelay(pdMS_TO_TICKS(1));
	}
}

//-------------------------------------------------------------------------------/

void HandleClientRequestSdCard(ClientRequestInformation *clientRequest){
	if(xSemaphoreTake(clientRequestMutex, 0) == pdTRUE){
		if(clientRequest->clientState == CLIENT_REQUEST){
			switch(clientRequest->fileState){
				case READ_FILE_STATE:
					ReadFileInFolder(clientRequest);
					break;
				case WRITE_FILE_STATE:
					WriteFileInFolder(clientRequest);
					break;
				default:
					break;
			}
		}
		xSemaphoreGive(clientRequestMutex);
	}
}
void WriteFileInFolder(ClientRequestInformation *clientRequest){
	ImageRaw *raw;
	if(GetQueueImageRawReady(&wifiImageRaw, &raw, IMAGE_RAW_NO_BLOCK) == pdTRUE){
		if(clientRequest->block == 0)
			clientRequest->file = fopen(clientRequest->f.path, "wb");
		FatSdCardSpiCustomWriteState state = WriteSdCardSpiFileOptimized(clientRequest->file, raw->buffer, BYTE_IN_FRAME);
		PushQueueImageRawFree(&wifiImageRaw, raw, IMAGE_RAW_BLOCK);
		if(FAT_SD_CARD_SPI_CUSTOM_WRITE_OK == state && clientRequest->block != clientRequest->countMsg){
			clientRequest->block++;
			return;
		}		
		fclose(clientRequest->file);
		WifiMessage *msg; 	
		GetQueueWifiMessagePointerFree(&queueMsgSend, &msg, QUEUE_MESSAGE_WIFI_BLOCK);
		msg->length = 0;
		switch(state){
			case FAT_SD_CARD_SPI_CUSTOM_WRITE_OK:
			case FAT_SD_CARD_SPI_CUSTOM_WRITE_FAIL_CAUSE_WRITE_FILE_ERROR:
			case FAT_SD_CARD_SPI_CUSTOM_WRITE_FAIL_CAUSE_PARTIAL_WRITE:
				msg->length += AddUlongToMessage(msg->buffer + msg->length, WRITE_FILE_ERROR, state);
				if(xSemaphoreTake(sdMemoryInfoMutex, portMAX_DELAY) == pdTRUE){
					GetSdCardInfo(&sdMemoryInfo.total, &sdMemoryInfo.free, &sdMemoryInfo.used);
					msg->length += AddUlongToMessage(msg->buffer, SD_CARD_USED, (uint32_t) sdMemoryInfo.used / 1024 / 1024);
				}
			case FAT_SD_CARD_SPI_CUSTOM_WRITE_FAIL_CAUSE_OPEN_FILE_FAIL:
				msg->length += AddUlongToMessage(msg->buffer + msg->length, TOTAL_BLOCK_IN_IN_FILE, clientRequest->countMsg);
			default:
				break;
		}
		PushQueueWifiMessageMsgReady(&queueMsgSend, msg, QUEUE_MESSAGE_WIFI_BLOCK);
	}
}

void HandleMessage(WifiMessage *message, ClientRequestInformation *clientRequest){
	HeaderFieldMessage info;
	clientRequest->clientState = CLIENT_REQUEST;
	clientRequest->mSocket = message->client;
	GetFieldInMessage( message->buffer, &info);
	
	//ESP_LOGE("Size", "lu", msg->length);
	/*
	for(uint i = 0; i < 4; i++)
		printf("0x%02X\n", message->buffer[i]);
	printf("%llu\n", (unsigned long long) info.code);
	*/
	switch(info.code){
		case LIST_FILE_IN_DIRECTORY_IMAGE_RAW_FOLDER:
			ReadListFileRequest(clientRequest);
			break;
		case READ_FILE_IN_LIST:
			ReadFileRequest(clientRequest, message->buffer + CONTENT_FIELD_MESSAGE_START, info.length);
			break;
		case WRITE_BLOCK_FILE: {
			uint32_t sizeName = info.length;
			GetFieldInMessage( message->buffer + CONTENT_FIELD_MESSAGE_START + sizeName, &info);
			if(info.code == TOTAL_BLOCK_IN_IN_FILE){
				uint32_t countMsg = GetUlongInMessage(message->buffer + CONTENT_FIELD_MESSAGE_START + sizeName + CONTENT_FIELD_MESSAGE_START);
				WriteFileRequest(clientRequest, (char*) message->buffer + CONTENT_FIELD_MESSAGE_START, sizeName, countMsg);
			}
			break;
		}
		case WRITE_BLOCK_MSG: {
			uint32_t indexMsg = GetUlongInMessage(message->buffer + CONTENT_FIELD_MESSAGE_START);
			PushBlockInQueueBlock(clientRequest, message->buffer + info.length, indexMsg);
			break;
		}
	}
}

void PushBlockInQueueBlock(ClientRequestInformation *clientRequest, uint8_t *data, uint32_t indexMsg){
	if(xSemaphoreTake(clientRequestMutex, portMAX_DELAY) == pdTRUE){
		if(indexMsg == clientRequest->msgWrite){
			clientRequest->msgWrite++;
			ImageRaw *raw;
			if(GetQueueImageRawFree(&wifiImageRaw, &raw, IMAGE_RAW_BLOCK) == pdTRUE){
				memcpy(raw->buffer, data, BYTE_IN_FRAME);
				PushQueueImageRawReady(&wifiImageRaw, raw, IMAGE_RAW_BLOCK);
			}
			xSemaphoreGive(clientRequestMutex);
		}
	}
}

void WriteFileRequest(ClientRequestInformation *clientRequest, char *nameFile, uint32_t size, uint32_t countMsg){
	if(xSemaphoreTake(clientRequestMutex, portMAX_DELAY) == pdTRUE){
		xSemaphoreGive(clientRequestMutex);
		char *name = clientRequest->f.path;;
		memcpy(name, PATH_DIR_IMAGE_RAW, strlen(PATH_DIR_IMAGE_RAW));
		memcpy(name + strlen(PATH_DIR_IMAGE_RAW), nameFile, size);
		name[strlen(PATH_DIR_IMAGE_RAW) + size] = '\0'; 
		clientRequest->fileState = WRITE_FILE_STATE;
		clientRequest->codeMsg = esp_random();
		clientRequest->block = 0;
		clientRequest->countMsg = countMsg;
		clientRequest->msgWrite = 0;
		WifiMessage *msg;
		if(GetQueueWifiMessagePointerFree(&queueMsgSend, &msg, QUEUE_MESSAGE_WIFI_BLOCK) == pdTRUE){
			uint32_t size = AddUlongToMessage(msg->buffer, WRITE_BLOCK_SETUP_DONE, clientRequest->codeMsg);
			msg->length = size;
			msg->client = clientRequest->mSocket;
			PushQueueWifiMessageMsgReady(&queueMsgSend, msg, QUEUE_MESSAGE_WIFI_BLOCK);
		}
		xSemaphoreGive(clientRequestMutex);
	}
}

void ReadFileRequest(ClientRequestInformation *clientRequest, uint8_t *nextDataPacket, uint32_t size){
	if(xSemaphoreTake(clientRequestMutex, portMAX_DELAY) == pdTRUE){
		char *name = clientRequest->f.path;
		memcpy(name, PATH_DIR_IMAGE_RAW, strlen(PATH_DIR_IMAGE_RAW));
		memcpy(name + strlen(PATH_DIR_IMAGE_RAW), nextDataPacket, size);
		name[strlen(PATH_DIR_IMAGE_RAW) + size] = '\0'; 
		clientRequest->fileState = READ_FILE_STATE;
		clientRequest->codeMsg = esp_random();
		clientRequest->block = 0;
		WifiMessage *msg;
		if(GetQueueWifiMessagePointerFree(&queueMsgSend, &msg, QUEUE_MESSAGE_WIFI_BLOCK) == pdTRUE){
			uint32_t size = AddUlongToMessage(msg->buffer, READ_FILE_SETUP_DONE, clientRequest->codeMsg);
			msg->length = size;
			msg->client = clientRequest->mSocket;
			PushQueueWifiMessageMsgReady(&queueMsgSend, msg, QUEUE_MESSAGE_WIFI_BLOCK);
		}
		xSemaphoreGive(clientRequestMutex);
	}
}

void ReadListFileRequest(ClientRequestInformation *clientRequest){
	if(xSemaphoreTake(listMuxtex, portMAX_DELAY) == pdTRUE){
		ReadListFileInImageRawFolder(clientRequest, &list);
		xSemaphoreGive(listMuxtex);
	}
}

void SetDoneHandleRequestClineInfomation(ClientRequestInformation *clientRequest){
	clientRequest->clientState = DONE_CLIENT_REQUEST;
}
void ReadFileInFolder(ClientRequestInformation *clientRequest){
	if(clientRequest->block == 0)
		clientRequest->file = fopen((const char*)clientRequest->f.path, "rb");
	WifiMessage *msg;
	if(GetQueueWifiMessagePointerFree(&queueMsgSend, &msg, QUEUE_MESSAGE_WIFI_BLOCK) == pdTRUE){
		msg->client = clientRequest->mSocket;
		FatSdCardSpiCustomCopyState sdCardCopyState  = CopySdCardSpiFile(clientRequest->file, msg->buffer + HEADER_WIFI_MESSAGE, BYTE_IN_FRAME);
		switch (sdCardCopyState){
			case FAT_SD_CARD_SPI_CUSTOM_COPY_OK:
				msg->length = HEADER_WIFI_MESSAGE + BYTE_IN_FRAME;
				clientRequest->block = clientRequest->block + 1;
				AddUlongToMessage(msg->buffer, BLOCK_FILE, clientRequest->block);
				break;
			case FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_READ_FILE_ERROR:
			case FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_OPEN_FILE_FAIL:
				msg->length = HEADER_WIFI_MESSAGE;
				AddUlongToMessage(msg->buffer, READ_FILE_ERROR, clientRequest->block);
				SetDoneHandleRequestClineInfomation(clientRequest);
				fclose(clientRequest->file);
				break;
			case FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_FILE_END:
				msg->length = HEADER_WIFI_MESSAGE;
				//printf("%llu\n", (unsigned long long)clientRequest->block);
				AddUlongToMessage(msg->buffer, TOTAL_BLOCK_IN_IN_FILE, clientRequest->block);
				SetDoneHandleRequestClineInfomation(clientRequest);
				fclose(clientRequest->file);
				break;
		}
		PushQueueWifiMessageMsgReady(&queueMsgSend, msg, QUEUE_MESSAGE_WIFI_BLOCK);
	}
}

void ReadListFileInImageRawFolder(ClientRequestInformation *clientRequest, DirentLinkerList *list){
	ESP_LOGE("test", "read list");
	WifiMessage *msg;
	if(GetQueueWifiMessagePointerFree(&queueMsgSend, &msg, QUEUE_MESSAGE_WIFI_BLOCK) == pdTRUE){
		msg->client = clientRequest->mSocket;
		uint32_t length = AddUlongToMessage((uint8_t*)msg->buffer, LIST_FILE_IN_DIRECTORY_IMAGE_RAW_FOLDER, list->size);
		if(list->size != 0){
			length += AddListToMessage((uint8_t*)msg->buffer + length, NAME_FILE_IN_LIST, list);
		}
		msg->length = length;
		PushQueueWifiMessageMsgReady(&queueMsgSend, msg, QUEUE_MESSAGE_WIFI_BLOCK);
		SetDoneHandleRequestClineInfomation(clientRequest);
	}
}

void SetupParameter(){
	fps = FPS_DEFAULT;
	FileInfomationInit(&mClientRequestInformation.f, LIST_BUFFER_SIZE);
	SetDoneHandleRequestClineInfomation(&mClientRequestInformation);
}

void SemaphoreInit(){
	listMuxtex = xSemaphoreCreateMutex();
	clientRequestMutex = xSemaphoreCreateMutex();
	sdMemoryInfoMutex = xSemaphoreCreateMutex();
}

//-------------------------------------------------------------------------------------------------------------------/

void TcpServerConfig(){
	TcpCustomInit(&tcpServerSocket, PORT);
	TcpCustomSetId(HEADER_CODE);
	QueueWifiMessageInit(&queueMsgReceive, QUEUE_MESSAGE_WIFI_LENGTH, WIFI_MESSAGE_QUEUE_BUFFER_SIZE);
	QueueWifiMessageInit(&queueMsgSend, QUEUE_MESSAGE_WIFI_LENGTH, WIFI_MESSAGE_QUEUE_BUFFER_SIZE);
	QueueImageRawInit(&wifiImageRaw, QUEUE_MESSAGE_WIFI_LENGTH, LEDPANEL_WIDTH, LEDPANEL_HEIGTH);
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

void RequestReadFileAndPushInQueueImageRaw(FILE *file, uint32_t size, char *path, DirentLinkerList *list){
	ImageRaw *raw;
	if(GetQueueImageRawFree(&ledPanelImageRaw, &raw, DMA_MANAGER_TIME_NO_BLOCK) == pdTRUE){
		FatSdCardSpiCustomCopyState sdCardCopyState  = CopySdCardSpiFile(file, raw->buffer, size);
		switch (sdCardCopyState) {
			case FAT_SD_CARD_SPI_CUSTOM_COPY_OK: 
				PushQueueImageRawReady(&ledPanelImageRaw, raw, DMA_MANAGER_TIME_NO_BLOCK);
				break;
			case FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_OPEN_FILE_FAIL:
				vTaskDelete(NULL);
				break;
			case FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_READ_FILE_ERROR:
				vTaskDelete(NULL);
				break;
			case FAT_SD_CARD_SPI_CUSTOM_COPY_FAIL_CAUSE_FILE_END:
				if(file != NULL){
					fclose(file);
					file = NULL;
				}
				RandomImageRawName(path, list);
				file = fopen(path, "rb");
				PushQueueImageRawFree(&ledPanelImageRaw, raw, IMAGE_RAW_BLOCK);
				break;
		}
	}
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
}


void LedPanelTask(void *pvParameters){
	VectorGdmaDescriptorsNode *gdma = NULL;
	int64_t start, end;

	while (1) {
		start = esp_timer_get_time();
		GetDmaDescriptorReady(&gdma, DMA_MANAGER_TIME_BLOCK);
		LedPanelRestart(&ledPanelConfig, gdma);
		end = esp_timer_get_time();
		int64_t timeSetup = end - start;
		//ESP_LOGE("ledPanel", "Time: %lld us\n", timeSetup);
		if(SECOND_UINT/fps > timeSetup)
			vTaskDelay(pdMS_TO_TICKS(SECOND_UINT/fps - timeSetup));
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

void CreateTask(){
	xTaskCreatePinnedToCore(
        LedPanelTask,
        "LedPanelTask",
        2560,
        NULL,
        FREE_RTOS_PRIORITY_REAL_TIME,
        NULL,
        CORE_1
    );
    xTaskCreatePinnedToCore(
        CoverntImageToVectorGdmadescriptorsNodeTask,
        "CoverntImageToVectorGdmadescriptorsNodeTask",
        2560,
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
    /*
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
    */
}