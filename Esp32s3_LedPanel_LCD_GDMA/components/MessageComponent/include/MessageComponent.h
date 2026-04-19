#ifndef MESSAGE_COMPONENT
#define MESSAGE_COMPONENT

#include "DirentLinkerListComponent.h"
#include <stdint.h>

#define	CODE_FIELD_MESSAGE_SIZE														4
#define	LENGTH_FIELD_MESSAGE_SIZE													4

#define	CODE_FIELD_MESSAGE_START													0
#define	LENGTH_FIELD_MESSAGE_START													(CODE_FIELD_MESSAGE_START + CODE_FIELD_MESSAGE_SIZE)
#define CONTENT_FIELD_MESSAGE_START													(LENGTH_FIELD_MESSAGE_START + LENGTH_FIELD_MESSAGE_SIZE)

#define UINT_BYTE_MAX																0xFF
#define UINT_BIT																	8
#define BYTE_LENGTH_MAX																4

#define ULONG_MESSAGE_TYPE															4
#define ULONGLONG_MESSAGE_TYPE														8

typedef struct HeaderFieldMessage{
	uint32_t code;
	uint32_t length;
} HeaderFieldMessage;

uint32_t AddULongLongToMessage(uint8_t *buffer, uint32_t code, uint32_t value);
void GetFieldInMessage(uint8_t *buffer, HeaderFieldMessage *header);
uint32_t GetUlongInMessage(uint8_t *buffer);
uint32_t GetValueInMessage(uint8_t *buffer, uint32_t size);
uint32_t GetCodeMessage(uint8_t *buffer);
uint32_t GetLengthMessage(uint8_t *buffer);
uint32_t AddUlongToMessage(uint8_t *buffer, uint32_t code, uint32_t value);
void AddHeaderFieldMessage(uint8_t *buffer, uint32_t code, uint32_t length);
uint32_t AddListToMessage(uint8_t *buffer, uint32_t code, DirentLinkerList *list);
uint32_t AddStringToMessage(uint8_t *buffer, uint32_t code, char *path);
uint8_t GetByteInValue(uint32_t value, uint32_t size, uint32_t index);

#endif