#include "TCPserver.h"
#include "chuniUSB-HID.h"

TCPPACKAGE TCPPakage;
TCPDATALIST Get;
TCPDATALIST Send;

HANDLE AimeLED;
DWORD __stdcall AimeLEDThread(void* pParam);
HANDLE AimeCard;
DWORD __stdcall AimeCardThread(void* pParam);

struct aimeinputdata
{
    unsigned char Statue;
    unsigned char ID[10];
};
int inputsize = sizeof(aimeinputdata);

struct aimeoutputdata
{
    unsigned char RGB[3];
};
int outputsize = sizeof(aimeoutputdata);


extern "C" void DataManage_Init(uint8_t* realaimeFlag);

void DataManage_Init(uint8_t* realaimeFlag)
{
	//初始化服务器
    TCPPakage.Init(TCP_SERVER, 60123, 600);

	//启动多线程
    unsigned long ulThreadId;
    //创建接收客户端请求线程
    AimeLED = CreateThread(NULL, 0, AimeLEDThread, NULL, 0, &ulThreadId);
    if (NULL == AimeLED)
    {
        std::cout << "AimeLED Thread create error" << std::endl;
    }
    else
    {
        CloseHandle(AimeLED);
    }

    AimeCard = CreateThread(NULL, 0, AimeCardThread, realaimeFlag, 0, &ulThreadId);
    if (NULL == AimeCard)
    {
        std::cout << "AimeCard Thread create error" << std::endl;
    }
    else
    {
        CloseHandle(AimeCard);
    }
}

//多线程功能:
/*
* 获取读卡器的RGB信息
* 定时传出读卡器的卡号信息
*/
DWORD __stdcall AimeLEDThread(void* pParam)
{
    while (1)
    {
        TCPPakage.While(&Get, &Send);
        while (Get.size() != 0)
        {
            if(Get.begin()->length == outputsize)
            {
                struct aimeoutputdata* AimeOut = (struct aimeoutputdata*)Get.begin()->Data;
                HIDUSB_SetReaderLED(AimeOut->RGB);
            }
            Get.erase(Get.begin());
        }
    }
    return 0;
}

DWORD __stdcall AimeCardThread(void* pParam)
{
    struct aimeinputdata AimeInput;
    struct TCPDATA TCPData;
    bool AimeFlag = (*((uint8_t*)(pParam))) == 0;
    while (1)
    {
        AimeInput.Statue = HIDUSB_GetCardStatue(AimeInput.ID);

        if(AimeFlag)
        {
            if (AimeInput.Statue == 1)
            {
                AimeInput.Statue = 2;
                int i;
                for (i = 0; i < 8; i++)
                {
                    AimeInput.ID[i] = AimeInput.ID[i + 2];
                }
            }
        }

        TCPData.BadCount = 0;
        TCPData.IPAddress = TCP_IPADDRESS;
        TCPData.length = inputsize;
        memcpy(TCPData.Data, (unsigned char*)&AimeInput, inputsize);
        Send.push_back(TCPData);
        Sleep(500);
        printf("GetCardStatue%d", AimeInput.Statue);
    }
    return 0;
}
