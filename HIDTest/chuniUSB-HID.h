#pragma once
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>

#include <hidsdi.h>
#include <setupapi.h>


int HIDUSB_Init(void);
int HIDUSB_Run(void);

uint8_t* HIDUSB_GetKeyValue(void);
uint8_t HIDUSB_GetButton(void);
uint8_t HIDUSB_GetBeams(void);
uint16_t HIDUSB_GetCoins(void);

uint8_t HIDUSB_GetCardStatue(unsigned char* id);

int HIDUSB_SetLedValue(uint8_t* p_buff);
int HIDUSB_SetReaderLED(uint8_t* p_buff);

void DelayInit(uint8_t delay);

uint32_t HIDUSB_PowerDebug(void);
