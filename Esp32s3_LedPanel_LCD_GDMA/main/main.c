#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
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
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "esp_wifi_types_generic.h"
#include "ff.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/spi_types.h"
#include "lwip/sockets.h"
#include "nvs_flash.h"
#include "portmacro.h"
#include "soc/gpio_num.h"
#include "FatSdCardSpiCustom.h"
#include "sys/dirent.h"
#include "esp_random.h"
#include "sys/errno.h"

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
#define HEADER_WIFI_MESSAGE															(LENGTH_FIELD_MESSAGE_SIZE + CODE_FIELD_MESSAGE_SIZE + ULONG_MESSAGE_TYPE)
#define BYTE_IN_FRAME																(LEDPANEL_WIDTH * LEDPANEL_HEIGTH * HUB75E_LUT_COLOR)		
#define SD_CARD_FREQ																10000
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
	HEADER_CODE 							= 0x89,
	LIST_FILE_IN_DIRECTORY_IMAGE_RAW_FOLDER = 0x33,
	NAME_FILE_IN_LIST						= 0x53,
	BLOCK_FILE								= 0x77,
	TOTAL_BLOCK_IN_IN_FILE					= 0x99
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

typedef enum CopyState{
	COPIED  ,
	PENDING_COPY
}CopyState;
//-------------------------------------------------------------------------------/

typedef struct ClientRequestInformation{
	FileInfomation fileInformation;
	FileCommunicationState fileState;
	ClientRequestState clientState;
	int mSocket;
	uint32_t block;
	uint32_t codeMsg;
} ClientRequestInformation;

//-------------------------------------------------------------------------------/

QueueImageRaw ledPanelImageRaw;
const char pathImageRawFolder[] = FAT_SD_CARD_SPI_CUSTOM_MOUSNT_PATH "/" PATH_DIR_IMAGE_RAW;
LedPanelConfig ledPanelConfig;

FatSdCardSpiCustomConfig sdCardConfig;
SemaphoreHandle_t queueWifiMessageReceiveMutex;
SemaphoreHandle_t queueWifiMessageSendMutex;
SemaphoreHandle_t clientRequestInformationMutex;
uint32_t fps;
struct FileInfomation ledPanelFile; 
uint32_t timeLedPanelUpdate;
TaskHandle_t coverntImageToVectorGdmadescriptorsNodeHandle = NULL;
TaskHandle_t tcpServerReceiveTaskHandle = NULL;
TaskHandle_t tcpServerSendTaskHandle = NULL;
TaskHandle_t handleWifiMessageTaskHandle = NULL;
int tcpServerSocket;
WifiMessageQueue queueMessageReceive;
WifiMessageQueue queueMessageSend;
ClientRequestInformation mClientRequestInformation;

//-------------------------------------------------------------------------------/
void HandleClientRequest(ClientRequestInformation *clientRequest);
void ReadFileRequest(uint8_t *path, uint32_t length, ClientRequestInformation *clientRequest);
int GetWifiMessageHeader(int *socket, uint8_t *buffer, uint32_t size);
int GetWifiMessageContent(int *socket, uint8_t *buffer, uint32_t size);
void TcpServerConfig();
void WifiConfig();
RandomImageRawNameState RandomImageRawName(char *path);
void LedPanelConfigInit(LedPanelConfig *config);
void FatSdCardSpiCustomConfigInit(FatSdCardSpiCustomConfig *config);
void LedPanelTask(void *pvParameters);
void CoverntImageToVectorGdmadescriptorsNodeTask(void *pvParameters);
void SdCardSpiTask(void *pvParameters);
void TcpServerReceiveTask(void *pvParameter);
void TcpServerSendTask(void* pvParameter);
void SemaphoreInit();
void CreateTask();
void SetupParameter();
void QueueWifiMessageConfig();
int  TcpSendBlock(int *mSocket, uint8_t *buffer, uint32_t size, esp_task_wdt_user_handle_t *wdtUser);
void HandleWifiMessageTask(void* pvParameter);

void HandleMessage(WifiMessage *message, ClientRequestInformation *clientRequest);
void ReadListFileRequest(ClientRequestInformation *clientRequest);
void ReadListFileInImageRawFolder(ClientRequestInformation *clientRequest);
void ReadListFileRequest(ClientRequestInformation *clientRequest);
void SetDoneHandleRequestClineInfomation(ClientRequestInformation *clientRequest);
void ReadFileInFolder(ClientRequestInformation *clientRequest);
ClientRequestState GetClientRequestState(ClientRequestInformation *clientInformation);
FileCommunicationState GetClientFileCommunicationState(ClientRequestInformation *clientInformation);
void RequestHandleClientRequest(ClientRequestInformation *source, ClientRequestInformation *temp, CopyState *copyState);
BaseType_t RequestReadFileAndPushInQueueImageRaw(FileInfomation *file, uint32_t size);
//-------------------------------------------------------------------------------/

void app_main(void)
{
	SetupParameter();
	ESP_LOGI(TAG_APP_MAIN, APP_MAIN_INIT);
	QueueWifiMessageConfig();
	WifiConfig();
	TcpServerConfig();
	FatSdCardSpiCustomConfigInit(&sdCardConfig);
	LedPanelConfigInit(&ledPanelConfig);
	FileInfomationInit(&ledPanelFile, SIZE_NAME);
	FileInfomationInit(&mClientRequestInformation.fileInformation, SIZE_NAME);
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
        FREE_RTOS_PRIORITY_HIGH,
        &coverntImageToVectorGdmadescriptorsNodeHandle,
        CORE_1
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
        &tcpServerReceiveTaskHandle,
        CORE_0
    );
    xTaskCreatePinnedToCore(
        TcpServerSendTask,
        "TcpServerSendTask",
        3072,
        NULL,
        FREE_RTOS_PRIORITY_HIGH,
        &tcpServerSendTaskHandle,
        CORE_0
    );
    xTaskCreatePinnedToCore(
        HandleWifiMessageTask,
        "HandleWifiMessageTask",
        3072,
        NULL,
        FREE_RTOS_PRIORITY_MEDIUM,
        &handleWifiMessageTaskHandle,
        CORE_1
    );
}

void HandleWifiMessageTask(void* pvParameter){
	WifiMessage *message;
	esp_task_wdt_user_handle_t wdtUser;
	QueueWifiMessageStateEnum queueState;
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);	
		queueState = QUEUE_WIFI_MESSAGE_NORMAL;
		esp_task_wdt_add_user("HandleWifiMessageTask", &wdtUser);
		while(queueState != QUEUE_WIFI_MESSAGE_EMPTY){
			if(xSemaphoreTake(queueWifiMessageReceiveMutex, portMAX_DELAY) == pdTRUE){
				queueState = QueueWifiMessageState(&queueMessageReceive);
				if(queueState != QUEUE_WIFI_MESSAGE_EMPTY){
					message = PeekHeadQueueWifiMessage(&queueMessageReceive);
					ESP_LOGE("Data receive", "Receive %lu data", (unsigned long) message->length);
					for(uint32_t i = 0; i < message->length; i++)
						ESP_LOGE("Data receive", "%02X ", message->buffer[i]);
					HandleMessage(message, &mClientRequestInformation);
					QueueWifiMessagePop(&queueMessageReceive);
					esp_task_wdt_reset_user(wdtUser);
				}
				xSemaphoreGive(queueWifiMessageReceiveMutex);
			}
		}
		esp_task_wdt_delete_user(wdtUser);
	}
}

void TcpServerSendTask(void* pvParameter){
	int headerSizeSend;
	uint32_t headerSize;
	WifiMessage *wifiMessage;
	uint8_t *buffer;
	uint8_t headerBuffer[HEADER_WIFI_MESSAGE];
	QueueWifiMessageStateEnum queueState;
	esp_task_wdt_user_handle_t wdtUser;
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		queueState = QUEUE_WIFI_MESSAGE_NORMAL;
		esp_task_wdt_add_user("HandleWifiMessageTask", &wdtUser);
		while(queueState != QUEUE_WIFI_MESSAGE_EMPTY){
			if(xSemaphoreTake(queueWifiMessageSendMutex, portMAX_DELAY) == pdTRUE){
				queueState = QueueWifiMessageState(&queueMessageSend);
				if(queueState != QUEUE_WIFI_MESSAGE_EMPTY){
					wifiMessage = PeekHeadQueueWifiMessage(&queueMessageSend);
					buffer = wifiMessage->buffer;
					headerSize = AddUlongToMessage(headerBuffer, HEADER_CODE, wifiMessage->length);
					headerSizeSend = send(wifiMessage->client, headerBuffer, headerSize, TCP_SEND_DATA_BLOCKING);
					
					if(headerSize == headerSizeSend){
						TcpSendBlock(&wifiMessage->client, buffer, wifiMessage->length, &wdtUser);
					}
					ESP_LOGE("Count message send", "Send message!");
					QueueWifiMessagePop(&queueMessageSend);
				}
				xSemaphoreGive(queueWifiMessageSendMutex);
			}
		}
		esp_task_wdt_delete_user(wdtUser);
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
    uint8_t *buffer = NULL;
    eTaskState taskState;
    WifiMessage *wifiMessage = NULL;
    int headerSize;
    uint32_t contentSize;
    uint8_t headerBuffer[HEADER_WIFI_MESSAGE];
    int contentReceive;
    HeaderFieldMessage infoMessage;
    uint32_t countNotify;
    QueueWifiMessageStateEnum queueReceveState = QUEUE_WIFI_MESSAGE_NORMAL;
	while(1){
		TcpCustomAccept(&tcpServerSocket, &sourceAddr, &addrLen, &clientSocket);
		if(clientSocket < 0){
            ESP_LOGE(TAG_TCP_SERVER_RECEIVE_TASK, "Unable to accept connection: errno %d", errno);
            break;
		}
		TcpServerSetKeepAlive(&clientSocket, &keepAlive, &keepIdle, &keepInterval, &keepCount);
		
		while(1){
			taskState = eReady;
			if(xSemaphoreTake(queueWifiMessageReceiveMutex, portMAX_DELAY) == pdTRUE){
				wifiMessage = PeekTailQueueWifiMessage(&queueMessageReceive);
				xSemaphoreGive(queueWifiMessageReceiveMutex);
				buffer = wifiMessage->buffer;
			}
			headerSize = GetWifiMessageHeader(&clientSocket, headerBuffer, HEADER_WIFI_MESSAGE);
			
			if(headerSize == EAI_AGAIN || headerSize == ETIMEDOUT){
				ESP_LOGE(TAG_TCP_SERVER_RECEIVE_TASK, "Client timeout!");
				break;
			} else if(headerSize == 0){
				ESP_LOGE(TAG_TCP_SERVER_RECEIVE_TASK, "Client disconnect!");
				break;
			}
			
			GetFieldInMessage(headerBuffer, &infoMessage);
			if(infoMessage.code != HEADER_CODE){
				ESP_LOGE(TAG_TCP_SERVER_RECEIVE_TASK, "Header invalid!");
				continue;
			}
			contentSize = GetUlongInMessage(headerBuffer);
			
			if(contentSize == 0){
				ESP_LOGE(TAG_TCP_SERVER_RECEIVE_TASK, "Size Message invalid!");
				continue;
			}
			contentReceive = GetWifiMessageContent(&clientSocket, buffer, contentSize);
			if(contentReceive != contentSize){
				ESP_LOGE(TAG_TCP_SERVER_RECEIVE_TASK, "Message missing data!");
				break;
			}
			ESP_LOGE("PACKET", "PACKET READY");
			wifiMessage->client = clientSocket;
			wifiMessage->length = contentReceive;
			do{
				if(xSemaphoreTake(queueWifiMessageReceiveMutex, portMAX_DELAY) == pdTRUE){
					queueReceveState = QueueWifiMessageState(&queueMessageReceive);
					if(queueReceveState == QUEUE_WIFI_MESSAGE_FULL){
						taskState = eBlocked;
					} else{
						QueueWifiMessagePush(&queueMessageReceive);
						xTaskNotifyGive(handleWifiMessageTaskHandle);
					}
					xSemaphoreGive(queueWifiMessageReceiveMutex);
				}

				if(taskState == eBlocked)
					xTaskNotifyWait(0, UINT32_MAX, &countNotify, portMAX_DELAY);	
			}
			while(queueReceveState == QUEUE_WIFI_MESSAGE_FULL);
		}
		close(clientSocket);
	}
}

void LedPanelTask(void *pvParameters){
	VectorGdmaDescriptorsNode *gdmaCurrent = NULL;
	VectorGdmaDescriptorsNode *gdmaNew = NULL;
	while (1) {
		int64_t start = esp_timer_get_time();
		gdmaCurrent = gdmaNew;
		GetDmaDescriptorReady(&gdmaNew, DMA_MANAGER_TIME_BLOCK);
		LedPanelStartTransmit(&ledPanelConfig, gdmaNew);

		
		if(gdmaCurrent != NULL)
			PushDmaDescriptorFree(gdmaCurrent, DMA_MANAGER_TIME_BLOCK);
		int64_t end = esp_timer_get_time();
		ESP_LOGE("led", "StartTransmit cost: %lld us", end - start);
		vTaskDelay(pdMS_TO_TICKS(SECOND_UINT/fps));
	}
}


void CoverntImageToVectorGdmadescriptorsNodeTask(void *pvParameters){
	VectorGdmaDescriptorsNode *gdma = NULL;
	ImageRaw *raw = NULL;
	while (1) {
		/*
		UBaseType_t stack_remain = uxTaskGetStackHighWaterMark(NULL);
		
		ESP_LOGE("CoverntImageToVectorGdmadescriptorsNodeTask", "Remaining stack: %d words (~%d bytes)", 
         stack_remain, stack_remain * 4);
         */
	    if (gdma == NULL) {
	        GetDmaDescriptorFree(&gdma, DMA_MANAGER_TIME_BLOCK);
	    }
	
	    if (raw == NULL) {
	        GetQueueImageRawReady(&ledPanelImageRaw, &raw, IMAGE_RAW_BLOCK);
	    }
	

		if (gdma != NULL && raw != NULL) {
		
		    int64_t start = esp_timer_get_time();
		
		    ESP_LOGI("led", "start convert");
		
		    LedPanelConvertFrameData(gdma, 
		                             &ledPanelConfig.style, 
		                             &ledPanelImageRaw.config, 
		                             raw);
		
		    int64_t end = esp_timer_get_time();
		
		    ESP_LOGE("convert", "Convert time: %lld us (~%lld ms)", 
		             end - start, 
		             (end - start) / 1000);
		
		    PushDmaDescriptorReady(gdma, DMA_MANAGER_TIME_BLOCK);
		    PushQueueImageRawFree(&ledPanelImageRaw, raw, IMAGE_RAW_BLOCK);
		
		    gdma = NULL;
		    raw = NULL;
		}
	}
}

void SdCardSpiTask(void *pvParameters){
	ClientRequestInformation client;
	CopyState mCopyState = PENDING_COPY;
	while (1) {
		UBaseType_t stack_remain = uxTaskGetStackHighWaterMark(NULL);
		
		//ESP_LOGE("SdCardSpiTask", "Remaining stack: %d words (~%d bytes)", stack_remain, stack_remain * 4); 
		RequestReadFileAndPushInQueueImageRaw(&ledPanelFile, BYTE_IN_FRAME);
		//RequestHandleClientRequest(&mClientRequestInformation, &client, &mCopyState);
		portYIELD();
	}
}

//-------------------------------------------------------------------------------/
BaseType_t RequestReadFileAndPushInQueueImageRaw(FileInfomation *file, uint32_t size){
	BaseType_t result = pdFALSE;
	ImageRaw *raw;
	if(GetQueueImageRawFree(&ledPanelImageRaw, &raw, DMA_MANAGER_TIME_NO_BLOCK) == pdTRUE){
		int64_t start = esp_timer_get_time();
		if(FileInfomationNameCheck(file) == FILE_INFOMATION_NAME_EMPTY)
			RandomImageRawName(file->path);
			
		FatSdCardSpiCustomCopyState sdCardCopyState  = CopySdCardSpiFile(file->path, raw->buffer, size, file->offset);
		if(sdCardCopyState == FAT_SD_CARD_SPI_CUSTOM_COPY_OK){
			file->offset += size;
			result = pdTRUE; 
			PushQueueImageRawReady(&ledPanelImageRaw, raw, DMA_MANAGER_TIME_NO_BLOCK);
		}
		else{
			SetFileInfomationEmty(file);
			PushQueueImageRawFree(&ledPanelImageRaw, raw, DMA_MANAGER_TIME_NO_BLOCK);
		}
		int64_t end = esp_timer_get_time();
		
		ESP_LOGE("sd card", "read time: %lld us (~%lld ms)", 
		             end - start, 
		             (end - start) / 1000);
	}
	return result;
}
void RequestHandleClientRequest(ClientRequestInformation *source, ClientRequestInformation *temp, CopyState *copyState){
	if(xSemaphoreTake(clientRequestInformationMutex, NO_DELAY) == pdTRUE){
		if(GetClientRequestState(source) == CLIENT_REQUEST && *copyState == PENDING_COPY){
			*temp = *source;
			*copyState = COPIED;
		}
		xSemaphoreGive(clientRequestInformationMutex);
	}
	if(*copyState == COPIED){
		HandleClientRequest(temp);
	}
	if(GetClientRequestState(temp) == DONE_CLIENT_REQUEST && xSemaphoreTake(clientRequestInformationMutex, NO_DELAY) == pdTRUE){
		source->clientState = DONE_CLIENT_REQUEST;
		*copyState = PENDING_COPY;
		xSemaphoreGive(clientRequestInformationMutex);
	}
}
void HandleClientRequest(ClientRequestInformation *clientRequest){
	FileCommunicationState state = GetClientFileCommunicationState(clientRequest);
	if(state == READ_LIST_FILE_IN_DIRECTORY_STATE)
		ReadListFileInImageRawFolder(clientRequest);
	else if(state == READ_FILE_STATE)
		ReadFileInFolder(clientRequest);
}

void HandleMessage(WifiMessage *message, ClientRequestInformation *clientRequest){
	HeaderFieldMessage info;
	clientRequest->clientState = CLIENT_REQUEST;
	clientRequest->mSocket = message->client;
	GetFieldInMessage( message->buffer, &info);
	if(xSemaphoreTake(clientRequestInformationMutex, portMAX_DELAY) == pdTRUE){
		if(info.code == READ_LIST_FILE_IN_DIRECTORY_STATE)
			ReadListFileRequest(clientRequest);
		else if(info.code == READ_FILE_STATE){
			ReadFileRequest(message->buffer + CONTENT_FIELD_MESSAGE_START, info.length, clientRequest);
		}
		xSemaphoreGive(clientRequestInformationMutex);
	}
}

void ReadFileRequest(uint8_t *path, uint32_t length, ClientRequestInformation *clientRequest){
	clientRequest->fileState = READ_FILE_STATE;
	clientRequest->block = 0;
	clientRequest->codeMsg = esp_random();
	memcpy(mClientRequestInformation.fileInformation.path, PATH_DIR_IMAGE_RAW, strlen(PATH_DIR_IMAGE_RAW));
	memcpy(mClientRequestInformation.fileInformation.path + strlen(PATH_DIR_IMAGE_RAW), path, length);
	clientRequest->fileInformation.path[strlen(PATH_DIR_IMAGE_RAW) + length] = FAT_SD_CARD_SPI_CUSTOM_END_FILE;
	ESP_LOGE("path file request", "%s", clientRequest->fileInformation.path);

}

void ReadListFileRequest(ClientRequestInformation *clientRequest){
	clientRequest->fileState = READ_LIST_FILE_IN_DIRECTORY_STATE;
}

void SetDoneHandleRequestClineInfomation(ClientRequestInformation *clientRequest){
	clientRequest->clientState = DONE_CLIENT_REQUEST;
}
void ReadFileInFolder(ClientRequestInformation *clientRequest){
	WifiMessage *desWifi = NULL;
	if(xSemaphoreTake(queueWifiMessageSendMutex, NO_DELAY) == pdTRUE && QueueWifiMessageState(&queueMessageSend ) != QUEUE_WIFI_MESSAGE_FULL){
		desWifi = PeekTailQueueWifiMessage(&queueMessageSend);
		QueueWifiMessagePush(&queueMessageSend);
		xSemaphoreGive(queueWifiMessageSendMutex);
	} else 
		return;
	FatSdCardSpiCustomCopyState sdCardCopyState = CopySdCardSpiFile(clientRequest->fileInformation.path, desWifi->buffer + CONTENT_FIELD_MESSAGE_START, BYTE_IN_FRAME, clientRequest->fileInformation.offset);
	if(sdCardCopyState == FAT_SD_CARD_SPI_CUSTOM_COPY_OK){
		clientRequest->fileInformation.offset += BYTE_IN_FRAME;
		AddHeaderFieldMessage(desWifi->buffer, BLOCK_FILE, BYTE_IN_FRAME);
	} else {
		AddUlongToMessage(desWifi->buffer, TOTAL_BLOCK_IN_IN_FILE, clientRequest->fileInformation.offset / BYTE_IN_FRAME);
		SetDoneHandleRequestClineInfomation(clientRequest);
	}
}

void ReadListFileInImageRawFolder(ClientRequestInformation *clientRequest){
	DirentLinkerList list;
	GetListFileSdCardSPI(PATH_DIR_IMAGE_RAW, &list);
	if(xSemaphoreTake(queueWifiMessageSendMutex, portMAX_DELAY) == pdTRUE){
		WifiMessage *desWifi = PeekTailQueueWifiMessage(&queueMessageSend);
		desWifi->client = clientRequest->mSocket;
		uint32_t length = AddUlongToMessage((uint8_t*)desWifi->buffer, LIST_FILE_IN_DIRECTORY_IMAGE_RAW_FOLDER, list.size);
		if(list.size != 0){
			length += AddListToMessage((uint8_t*)desWifi->buffer + length, NAME_FILE_IN_LIST, &list);
		}
		desWifi->length = length;
		QueueWifiMessagePush(&queueMessageSend);
		xSemaphoreGive(queueWifiMessageSendMutex);
	}
	xTaskNotifyGive(tcpServerSendTaskHandle);
	SetDoneHandleRequestClineInfomation(clientRequest);
}

FileCommunicationState GetClientFileCommunicationState(ClientRequestInformation *clientInformation){
	return clientInformation->fileState;
}

ClientRequestState GetClientRequestState(ClientRequestInformation *clientInformation){
	return clientInformation->clientState;
}

int TcpSendBlock(int *mSocket, uint8_t *buffer, uint32_t size, esp_task_wdt_user_handle_t *wdtUser){
	int sendSize = 0;
	uint32_t dataReceiveSize = 0;
	uint32_t blockSize;
	while(sendSize < size){
		blockSize = (size - sendSize) > BLOCK_DATA_MAX ? BLOCK_DATA_MAX : (size - sendSize);
		dataReceiveSize = send(*mSocket, buffer + sendSize, blockSize, TCP_SEND_DATA_BLOCKING);
		esp_task_wdt_reset_user(*wdtUser);
		if(dataReceiveSize < 1)
			return sendSize;
		sendSize += dataReceiveSize;
	}
	return sendSize;
}

int GetWifiMessageHeader(int *socket, uint8_t *buffer, uint32_t size){
	return recv(*socket, buffer, size, TCP_RECEIVE_DATA_BLOCKING);
}
int GetWifiMessageContent(int *socket, uint8_t *buffer, uint32_t size){
	int receive = 0;
	uint32_t dataReceiveSize = 0;
	while(receive < size){
		dataReceiveSize = recv(*socket, buffer + receive, size, TCP_RECEIVE_DATA_BLOCKING);
		if(dataReceiveSize < 1)
			break;
		receive += dataReceiveSize;
	}
	return receive;
}

void TcpServerConfig(){
	TcpCustomInit(&tcpServerSocket, PORT);
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

void SetupParameter(){
	fps = FPS_DEFAULT;
	SetDoneHandleRequestClineInfomation(&mClientRequestInformation);
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
   	config->host.max_freq_khz = SD_CARD_FREQ;
    FatSdCardSpiCustomInit(config);
    
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
}

void QueueWifiMessageConfig(){
	QueueWifiMessageBufferInit(&queueMessageSend, QUEUE_MESSAGE_WIFI_LENGTH, WIFI_MESSAGE_QUEUE_BUFFER_SIZE);
	QueueWifiMessageBufferInit(&queueMessageReceive, QUEUE_MESSAGE_WIFI_LENGTH, WIFI_MESSAGE_QUEUE_BUFFER_SIZE);
}

void SemaphoreInit(){
	queueWifiMessageReceiveMutex = xSemaphoreCreateMutex();
	queueWifiMessageSendMutex = xSemaphoreCreateMutex();
	clientRequestInformationMutex = xSemaphoreCreateMutex();
}
