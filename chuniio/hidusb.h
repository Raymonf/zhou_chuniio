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

void HIDUSB_SetLedValue(uint8_t* p_buff);

