#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "MessageComponent.h"

void GetFieldInMessage(uint8_t *buffer, HeaderFieldMessage *header){
	header->code = GetValueInMessage(buffer, CODE_FIELD_MESSAGE_SIZE);
	header->length = GetValueInMessage(buffer + CODE_FIELD_MESSAGE_SIZE, LENGTH_FIELD_MESSAGE_SIZE);
}

uint32_t GetCodeMessage(uint8_t *buffer){
	uint32_t code = GetValueInMessage(buffer, CODE_FIELD_MESSAGE_SIZE);
	return code;
}
uint32_t GetLengthMessage(uint8_t *buffer){
	uint32_t length = GetValueInMessage(buffer, LENGTH_FIELD_MESSAGE_SIZE);
	return length;
}

uint32_t GetUlongInMessage(uint8_t *buffer){
	return GetValueInMessage(buffer + CONTENT_FIELD_MESSAGE_START, ULONG_MESSAGE_TYPE);
}

uint32_t GetValueInMessage(uint8_t *buffer, uint32_t size){
	uint32_t code = 0;
	for(uint32_t i = 0; i < size; i++)
		code |= ((uint32_t)buffer[i] << ((size - i - 1) * 8));
	return code;
}

uint32_t AddListToMessage(uint8_t *buffer, uint32_t code, DirentLinkerList *list){
	uint32_t length = 0;
	for(uint32_t i = 0; i < list->size; i++){
		length += AddStringToMessage(buffer + length, code, (char*)DirentLinkerListGetIndex(list, i)->buffer);
	}
	return length;
}

uint32_t AddULongLongToMessage(uint8_t *buffer, uint32_t code, uint32_t value){
	uint32_t length = CODE_FIELD_MESSAGE_SIZE + LENGTH_FIELD_MESSAGE_SIZE + ULONG_MESSAGE_TYPE;
	AddHeaderFieldMessage(buffer, code, ULONG_MESSAGE_TYPE);
	for(uint32_t i = 0; i < CODE_FIELD_MESSAGE_SIZE; i++)
		*(buffer + i + CONTENT_FIELD_MESSAGE_START) = GetByteInValue(value, ULONGLONG_MESSAGE_TYPE, i);
	return length;
}

uint32_t AddUlongToMessage(uint8_t *buffer, uint32_t code, uint32_t value){
	uint32_t length = CODE_FIELD_MESSAGE_SIZE + LENGTH_FIELD_MESSAGE_SIZE + ULONG_MESSAGE_TYPE;
	AddHeaderFieldMessage(buffer, code, ULONG_MESSAGE_TYPE);
	for(uint32_t i = 0; i < CODE_FIELD_MESSAGE_SIZE; i++)
		*(buffer + i + CONTENT_FIELD_MESSAGE_START) = GetByteInValue(value, ULONG_MESSAGE_TYPE, i);
	return length;
}

uint32_t AddStringToMessage(uint8_t *buffer, uint32_t code, char *path){
	uint32_t length = strlen(path);
	AddHeaderFieldMessage(buffer, code, length);
	memcpy(buffer + CONTENT_FIELD_MESSAGE_START, path, length);
	return CODE_FIELD_MESSAGE_SIZE + LENGTH_FIELD_MESSAGE_SIZE + length;
}

uint8_t GetByteInValue(uint32_t value, uint32_t size, uint32_t index){
	return (value >> (UINT_BIT * (size - index - 1))) & UINT_BYTE_MAX;
}

void AddHeaderFieldMessage(uint8_t *buffer, uint32_t code, uint32_t length){
	for(uint32_t i = 0; i < CODE_FIELD_MESSAGE_SIZE; i++)
		*(buffer + i + CODE_FIELD_MESSAGE_START) = GetByteInValue(code, CODE_FIELD_MESSAGE_SIZE, i);
	for(uint32_t i = 0; i < LENGTH_FIELD_MESSAGE_SIZE; i++)
		*(buffer + i + LENGTH_FIELD_MESSAGE_START) = GetByteInValue(length, LENGTH_FIELD_MESSAGE_SIZE, i);
}