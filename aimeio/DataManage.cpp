#include "TCPserver.h"

//#include "dprintf.h"

//#include <iostream>
//#include <fstream>
//#include <string>
//#include <windows.h>

//using namespace std;



TCPPACKAGE TCPPackage;
TCPDATALIST Get;
TCPDATALIST Send;

//output log 
//int logfile(string log,DWORD error)
//{
//	ofstream logfile;
//	logfile.open("log.txt", ios::app);
//	logfile << log  << error <<endl;
//	logfile.close();
//	return 0;
//}


struct aimeinputdata
{
	unsigned char Statue;
	unsigned char ID[10];
}AimeInput;

struct aimeoutputdata
{
	unsigned char RGB[3];
}AimeOutput;

int inputsize = sizeof(aimeinputdata);
int outputsize = sizeof(aimeoutputdata);

HANDLE AimeDM;
DWORD __stdcall AimeDMThread(void* pParam);

/*�߼�����*/
extern "C" void DataManageInit(void);

void DataManageInit(void)
{
	//��ʼ��TCP
	TCPPackage.Init(TCP_CLIENT, 60123, 2);

	//�������߳�
	unsigned long ulThreadId;
	//�������տͻ��������߳�
	AimeDM = CreateThread(NULL, 0, AimeDMThread, NULL, 0, &ulThreadId);
	if (NULL == AimeDM)
	{
		std::cout << "AimeDM Thread create error" << std::endl;
	}
	else
	{
		CloseHandle(AimeDM);
	}
}

DWORD __stdcall AimeDMThread(void* pParam)
{
	while (1)
	{
		TCPPackage.While(&Get, &Send);
		while (Get.size() != 0)
		{
			//printf("GetData,Size:%d\r\n", Get.size());
			if (Get.begin()->length == inputsize)
			{
				memcpy((unsigned char*)&AimeInput, Get.begin()->Data, inputsize);
				//printf("GetData\r\n");
			}
			Get.erase(Get.begin());
		}
	}
	return 0;
}


/*�ӿں���*/
extern "C" unsigned char DataManage_GetCard(unsigned char* id);
extern "C" void DataManage_InputRGB(unsigned char* rgb);


unsigned char DataManage_GetCard(unsigned char* id)
{
	memcpy(id, AimeInput.ID, 10);
	//logfile("Card Type is ",AimeInput.Statue);
	//logfile("Card ID is ", (int) id);
	return AimeInput.Statue;
}
void DataManage_InputRGB(unsigned char* rgb)
{
	memcpy(AimeOutput.RGB,rgb,3);

	TCPDATA tempData;
	tempData.BadCount = 0;
	tempData.IPAddress = TCP_IPADDRESS;
	tempData.length = (unsigned char)outputsize;
	memcpy(tempData.Data, (unsigned char*)&AimeOutput, outputsize);

	//printf("Send push Length:%02x,Data:%02x,%02x,%02x  ", tempData.length, tempData.Data[0], tempData.Data[1], tempData.Data[2]);
	Send.push_back(tempData);
}
