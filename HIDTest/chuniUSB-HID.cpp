#include <windows.h>

#include <process.h>
#include <stdbool.h>
#include <stdint.h>

#include <stdio.h>

#include <hidsdi.h>
#include <setupapi.h>

#include "chuniUSB-HID.h"



extern "C" int c_HIDUSB_Init(void);
extern "C" int c_HIDUSB_Run(void);

extern "C" uint8_t * c_HIDUSB_GetKeyValue(void);
extern "C" uint8_t c_HIDUSB_GetButton(void);
extern "C" uint8_t c_HIDUSB_GetBeams(void);
extern "C" uint16_t c_HIDUSB_GetCoins(void);
extern "C" uint8_t c_HIDUSB_GetCardStatue(unsigned char* id);

extern "C" int c_HIDUSB_SetLedValue(uint8_t * p_buff);
extern "C" int c_HIDUSB_SetReaderLED(uint8_t * p_buff);

extern "C" void c_DelayInit(uint8_t delay);

int c_HIDUSB_Init(void)
{
    return(HIDUSB_Init());
}

int c_HIDUSB_Run(void)
{
    return(HIDUSB_Run());
}

uint8_t* c_HIDUSB_GetKeyValue(void)
{
    return(HIDUSB_GetKeyValue());
}

uint8_t c_HIDUSB_GetButton(void)
{
    return(HIDUSB_GetButton());
}

uint8_t c_HIDUSB_GetBeams(void)
{
    return(HIDUSB_GetBeams());
}

uint16_t c_HIDUSB_GetCoins(void)
{
    return(HIDUSB_GetCoins());
}

int c_HIDUSB_SetLedValue(uint8_t* p_buff)
{
    return HIDUSB_SetLedValue(p_buff);
}

uint8_t c_HIDUSB_GetCardStatue(unsigned char* id)
{
    return HIDUSB_GetCardStatue(id);
}

int c_HIDUSB_SetReaderLED(uint8_t* p_buff)
{
    return HIDUSB_SetReaderLED(p_buff);
}


void c_DelayInit(uint8_t delay)
{
    DelayInit(delay);
}


wchar_t* char2wchar(const char* cchar)
{
    wchar_t* m_wchar;
    int len = MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), NULL, 0);
    m_wchar = new wchar_t[len + 1];
    MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), m_wchar, len);
    m_wchar[len] = '\0';
    return m_wchar;
}




struct inputdata
{
    unsigned char Address;
    unsigned char IRValue;
    unsigned char Buttons;
    unsigned char TouchData[32];
    unsigned char CardStatue;
    unsigned char CardID[10];
}InputData;
static const int InputDataSize = sizeof(inputdata);

struct outputdata_dasi
{
    unsigned char Address;
    unsigned char Touch[48];
    unsigned char Reader[3];
    unsigned char Empty[11];
}OutputDataDasi;
static const int OutputDataSize_Dasi = sizeof(outputdata_dasi);
struct outputdata_gaosan
{
    unsigned char Address;
    unsigned char Touch[62];
}OutputDataGaosan;
static const int OutputDataSize_Gaosan = sizeof(outputdata_gaosan);
//uint8_t TOUCHDELAYLENGTH=1;
//struct
//{
//    unsigned char Buffer[10][32];
//    unsigned char Index;
//}TouchDelay;
//#define LASTTOUCH(x) (x+1 == TOUCHDELAYLENGTH? 0:x+1)

static bool coin = false;
static uint16_t coins = 0;

static HANDLE hidHandle = NULL;

static int txcount = 0;

static unsigned int error_count = 0;
//debug
//static int errorcount = 0;

#define WC(x) x,(sizeof(x)/2)-1

static enum {
    NONE,
    GAOSAN,
    DASI
} ControllerType;

void DelayInit(uint8_t delay)
{
    //TouchDelay.Index = 0;
    //TOUCHDELAYLENGTH = delay;
    //printf("ZhouSensor Touch Delay is: %d ms", (delay-1) * 4);
}

int HIDUSB_Init(void)
{
    ULONG                     requiredLength;
    GUID                      hidGuid;
    SP_DEVICE_INTERFACE_DATA  devInfoData;
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    int                       deviceNo;
    bool                      result;

    ControllerType = NONE;

    deviceNo = 0;
    if (hidHandle != NULL)
    {
        CloseHandle(hidHandle);
    }
    hidHandle = NULL;
    devInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);


    HidD_GetHidGuid(&hidGuid);

    HDEVINFO hDevInfo = SetupDiGetClassDevs(&hidGuid, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

    if (hDevInfo == INVALID_HANDLE_VALUE) {
        //dprintf("USB- Fatal Error: SetupDiGetClassDevs Fail!!!\r\n");
        WriteConsole(hStdOut, WC(L"\rUSB- Fatal Error: SetupDiGetClassDevs Fail!!!"), NULL, NULL);
        return 1;
    }

    SetLastError(NO_ERROR);

    while (1) //Find Device
    {
        result = SetupDiEnumInterfaceDevice(hDevInfo, 0, &hidGuid, deviceNo, &devInfoData);

        if ((result == false) || (GetLastError() == ERROR_NO_MORE_ITEMS)) {    /* 出现ERROR_NO_MORE_ITEMS错误表示已经找完了所有的设备 */
            //dprintf("USB- Can not Find ZhouSensor ChuniTouchPad \r\n\r\n");
            WriteConsole(hStdOut, WC(L"\rUSB- Can not Find ZhouSensor ChuniTouchPad"), NULL, NULL);
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return 1;
        }

        requiredLength = 0;                                                                                       /* 先将变量置零，以便于下一步进行获取 */
        SetupDiGetInterfaceDeviceDetail(hDevInfo, &devInfoData, NULL, 0, &requiredLength, NULL);                  /* 第一次调用，为了获取requiredLength */
        PSP_INTERFACE_DEVICE_DETAIL_DATA devDetail = (SP_INTERFACE_DEVICE_DETAIL_DATA*)malloc(requiredLength);   /* 根据获取到的长度申请动态内存 */
        devDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);                                              /* 先对变量进行部分初始化 */
        result = SetupDiGetInterfaceDeviceDetail(hDevInfo, &devInfoData, devDetail, requiredLength, NULL, NULL);  /* 第二次调用，为了获取devDetail */

        //PSP_INTERFACE_DEVICE_DETAIL_DATA devDetail =NULL;
        //result = false;

        if (result == false) {
            WriteConsole(hStdOut, WC(L"\rUSB- Fatal Error: SetupDiGetInterfaceDeviceDetail fail!!!"), NULL, NULL);
            free(devDetail);
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return 1;
        }

        hidHandle = CreateFileW(devDetail->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        // FILE_SHARE_READ | FILE_SHARE_WRITE

        free(devDetail);

        if (hidHandle == INVALID_HANDLE_VALUE) {                               /* 系统会将部分HID设备设置成独占模式 */
            ++deviceNo;
            CloseHandle(hidHandle);
            continue;
        }

        _HIDD_ATTRIBUTES hidAttributes;

        result = HidD_GetAttributes(hidHandle, &hidAttributes);                /* 获取HID设备的属性 */

        if (result == false) {
            ++deviceNo;
            CloseHandle(hidHandle);
            continue;
        }

        if (hidAttributes.ProductID == 0x2001 && hidAttributes.VendorID == 0x1973)
        {
            WriteConsole(hStdOut, WC(L"\rUSB- Success Find ZhouSensor Chuni Touchpad[Ver 1.01]"), NULL, NULL);
            error_count = 0;
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return 0;
        }

        ++deviceNo;
    }

    //dprintf("USB- Unknow Error");
    //WriteConsole(hStdOut, WC(L"\rUSB- Unknow Error"), NULL, NULL);

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return 2;
}

int HIDUSB_Run(void)
{
    bool                      result;
    unsigned char buffer[64];
    DWORD returnlength;
    

    if (error_count > 0)
    {
        if (HIDUSB_Init() == 0)
        {
            error_count = 0;
        }
        else
        {
            Sleep(100);
            return 1;
        }
    }

    if (hidHandle == NULL)
        return 1;


    //Input
    result = ReadFile(hidHandle, buffer, 64, &returnlength, NULL);
    if (result != false)
    {
        switch (returnlength)
        {
        case 35:
            ControllerType = GAOSAN;
            break;
        case InputDataSize:
            ControllerType = DASI;
            break;
        }

        if (ControllerType != NONE)
        {
            for (int i = 0; i < 32; i++)
            {
                InputData.TouchData[31 - i] = buffer[i + 3];
            }
            InputData.Buttons = buffer[2];
            InputData.IRValue = buffer[1];

            if (ControllerType == DASI)
            {
                InputData.CardStatue = buffer[35];
                for (int i = 0; i < 10; i++)
                {
                    InputData.CardID[i] = buffer[i + 36];
                }
            }
            else
            {
                InputData.CardStatue = 0;
            }
            
            if (InputData.Buttons & 0x04) {
                if (!coin) {
                    coin = true;
                    coins++;
                }
            }
            else {
                coin = false;
            }
            error_count = 0;
        }
    }
    else
    {
        error_count++;
    }

    return 0;
}





uint8_t* HIDUSB_GetKeyValue(void)
{
    return InputData.TouchData;
}

uint8_t HIDUSB_GetButton(void)
{
    return(InputData.Buttons);
}

uint8_t HIDUSB_GetBeams(void)
{
    return(InputData.IRValue);
}

uint16_t HIDUSB_GetCoins(void)
{
    return(coins);
}

uint8_t HIDUSB_GetCardStatue(unsigned char* id)
{
    int i;
    for (i = 0; i < 10; i++)
    {
        id[i] = InputData.CardID[i];
    }
    return InputData.CardStatue;
}


int HIDUSB_SetLedValue(uint8_t* p_buff)//96input
{
    DWORD nothing;
    if (error_count > 0)
        return 1;
    if (hidHandle == NULL)
        return 1;


    if (ControllerType == NONE)
        return 0;


    switch (ControllerType)
    {
    case GAOSAN:
        OutputDataGaosan.Address = 0;

        for (int i = 0; i < 31; i++)
        {
            unsigned short temp = 0;
            //R
            temp |= ((unsigned short)(p_buff[i * 3 + 0] >> 2)) << 5;
            //G
            temp |= ((unsigned short)(p_buff[i * 3 + 1] >> 3)) << 11;
            //B
            temp |= ((unsigned short)(p_buff[i * 3 + 2] >> 3)) << 0;

            unsigned short* target = (unsigned short*)&OutputDataGaosan.Touch[i * 2];
            (*target) = temp;
        }

        if (WriteFile(hidHandle, (unsigned char*)&OutputDataGaosan, OutputDataSize_Gaosan, &nothing, NULL) == false)
        {
            printf("WriteData Error");
            error_count++;
        }
        break;

    case DASI:
        OutputDataDasi.Address = 0;

        for (int i = 0; i < 16; i++)
        {
            OutputDataDasi.Touch[i * 3] = (p_buff[i * 6 + 0] >> 4) | (p_buff[i * 6 + 1] & 0xF0);
            OutputDataDasi.Touch[i * 3 + 1] = (p_buff[i * 6 + 2] >> 4) | (p_buff[i * 6 + 3] & 0xF0);
            OutputDataDasi.Touch[i * 3 + 2] = (p_buff[i * 6 + 4] >> 4) | (p_buff[i * 6 + 5] & 0xF0);
        }

        if (WriteFile(hidHandle, (unsigned char*)&OutputDataDasi, OutputDataSize_Dasi, &nothing, NULL) == false)
        {
            printf("WriteData Error");
            error_count++;
        }
        break;
    }
    

    

    return 0;
}

#define READERLESS 16
int HIDUSB_SetReaderLED(uint8_t* p_buff)
{
    if (ControllerType != DASI)
        return 0;

    int i;
    if (p_buff[0] < READERLESS && p_buff[1] < READERLESS && p_buff[2] < READERLESS)
    {
        for (i = 0; i < 3; i++)
        {
            OutputDataDasi.Reader[i] = READERLESS;
        }
    }
    else
    {
        for (i = 0; i < 3; i++)
        {
            OutputDataDasi.Reader[i] = p_buff[i];
        }
    }
    
    return 0;
}

uint32_t HIDUSB_PowerDebug(void)
{
    //uint32_t power = *((uint32_t*)InputData.Power);
    //return power;
    return 0;
}
