#include <windows.h>

#include <process.h>
#include <stdbool.h>
#include <stdint.h>

#include <hidsdi.h>
#include <setupapi.h>

#include "hidusb.h"

extern "C" {
#include "dprintf.h"
}

extern "C" int c_HIDUSB_Init(void);
extern "C" int c_HIDUSB_Run(void);

extern "C" uint8_t* c_HIDUSB_GetKeyValue(void);
extern "C" uint8_t c_HIDUSB_GetButton(void);
extern "C" uint8_t c_HIDUSB_GetBeams(void);
extern "C" uint16_t c_HIDUSB_GetCoins(void);

extern "C" void c_HIDUSB_SetLedValue(uint8_t* p_buff);

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

void c_HIDUSB_SetLedValue(uint8_t* p_buff)
{
    HIDUSB_SetLedValue(p_buff);
}



static bool coin = false;
static uint16_t coins = 0;

static uint8_t key_buffer[32];
static uint8_t led_buffer[63];
static uint8_t button=0;
static uint8_t beams=0;


static int                       deviceNo;
static HANDLE                    hidHandle;
static GUID                      hidGuid;
static ULONG                     requiredLength;
static SP_DEVICE_INTERFACE_DATA  devInfoData;

static int txcount = 0;


#define WC(x) x,sizeof(x)

int HIDUSB_Init(void)
{
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    bool                      result;
    deviceNo = 0;
    hidHandle = NULL;
    devInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);


    HidD_GetHidGuid(&hidGuid);

    HDEVINFO hDevInfo = SetupDiGetClassDevs(&hidGuid, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

    if (hDevInfo == INVALID_HANDLE_VALUE) {
        //dprintf("USB- Fatal Error: SetupDiGetClassDevs Fail!!!\r\n");
        WriteConsole(hStdOut, WC(L"USB- Fatal Error: SetupDiGetClassDevs Fail!!!\r\n"), NULL, NULL);
        return 1;
    }

    SetLastError(NO_ERROR);

    while (1) //Find Device
    {
        result = SetupDiEnumInterfaceDevice(hDevInfo, 0, &hidGuid, deviceNo, &devInfoData);

        if ((result == false) || (GetLastError() == ERROR_NO_MORE_ITEMS)) {    /* 出现ERROR_NO_MORE_ITEMS错误表示已经找完了所有的设备 */
            //dprintf("USB- Can not Find ZhouSensor ChuniTouchPad \r\n\r\n");
            WriteConsole(hStdOut, WC(L"USB- Can not Find ZhouSensor ChuniTouchPad \r\n\r\n"), NULL, NULL);
            return 1;
        }

        requiredLength = 0;                                                                                       /* 先将变量置零，以便于下一步进行获取 */
        SetupDiGetInterfaceDeviceDetail(hDevInfo, &devInfoData, NULL, 0, &requiredLength, NULL);                  /* 第一次调用，为了获取requiredLength */
        PSP_INTERFACE_DEVICE_DETAIL_DATA devDetail = (SP_INTERFACE_DEVICE_DETAIL_DATA*)malloc(requiredLength);   /* 根据获取到的长度申请动态内存 */
        devDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);                                              /* 先对变量进行部分初始化 */
        result = SetupDiGetInterfaceDeviceDetail(hDevInfo, &devInfoData, devDetail, requiredLength, NULL, NULL);  /* 第二次调用，为了获取devDetail */

        if (result == false) {
            //dprintf("USB- Fatal Error: SetupDiGetInterfaceDeviceDetail fail!!!\r\n");
            WriteConsole(hStdOut, WC(L"USB- Fatal Error: SetupDiGetInterfaceDeviceDetail fail!!!\r\n"), NULL, NULL);
            free(devDetail);
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return 1;
        }

        hidHandle = CreateFile(devDetail->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

        free(devDetail);

        if (hidHandle == INVALID_HANDLE_VALUE) {                               /* 系统会将部分HID设备设置成独占模式 */
            ++deviceNo;
            continue;
        }

        _HIDD_ATTRIBUTES hidAttributes;

        result = HidD_GetAttributes(hidHandle, &hidAttributes);                /* 获取HID设备的属性 */

        if (result == false) {
            dprintf("USB- Fatal Error: HidD_GetAttributes fail!!!\r\n");
            WriteConsole(hStdOut, WC(L"USB- Fatal Error: HidD_GetAttributes fail!!!\r\n"), NULL, NULL);
            CloseHandle(hidHandle);
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return 1;
        }

        if (hidAttributes.ProductID == 0x2001 && hidAttributes.VendorID == 0x1973)
        {
            dprintf("USB- Success Find ZhouSensor Chuni Touchpad\r\n");
            WriteConsole(hStdOut, WC(L"USB- Success Find ZhouSensor Chuni Touchpad\r\n"), NULL, NULL);
            return 0;
        }
    }

    dprintf("USB- Unknow Error");
    WriteConsole(hStdOut, WC(L"USB- Unknow Error"), NULL, NULL);
    return 2;
}

int HIDUSB_Run(void)
{
    //Input
    bool                      result;
    unsigned char buffer[64];
    DWORD bufferl;
    result = ReadFile(hidHandle, buffer, 35, &bufferl, NULL);

    if ((result != false) && (bufferl == 35) && (buffer[0] == 0))
    {
        int i;
        for (i = 0; i < 32; i++)
        {
            key_buffer[i] = buffer[i + 3];
        }
        button = buffer[2];
        beams = buffer[1];

        if (button & 0x04) {
            if (!coin) {
                coin = true;
                coins++;
            }
        }
        else {
            coin = false;
        }
    }

    //Output
    txcount++;
    if (txcount >= 16)
    {
        txcount = 0;
        WriteFile(hidHandle, led_buffer, 63, NULL, NULL);
    }


    return 0;
}





uint8_t* HIDUSB_GetKeyValue(void)
{
    return(key_buffer);
}

uint8_t HIDUSB_GetButton(void)
{
    return(button);
}

uint8_t HIDUSB_GetBeams(void)
{
    return(beams);
}

uint16_t HIDUSB_GetCoins(void)
{
    return(coins);
}

void HIDUSB_SetLedValue(uint8_t* p_buff)
{
    int i;
    for (i = 0; i < 63; i++)
    {
        led_buffer[i] = *(p_buff + i);
    }
}
