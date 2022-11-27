#include "TCPserver.h"
#include <time.h>
#pragma comment(lib,"ws2_32.lib")  //��̬����ws2_32.lib

DWORD __stdcall AcceptThread(void* pParam);

bool TCPPACKAGE::Init(bool mode, int port, int timeoutsec)
{
	this->WorkMode = mode;
	this->PortNumber = port;
	this->Timeout = timeoutsec;
	sendFirst = true;
	DSInit(); //��ʼ�������־λ

	//����ģʽ��ʼ����Ӧ����
	if (mode == TCP_SERVER)
		return InitServer();
	else
		return InitClient();
}


bool TCPPACKAGE::InitServer(void) //��ʼ��������
{
	//����ֵ
	int reVal;

	//��ʼ��Windows Sockets DLL
	WSADATA  wsData;
	reVal = WSAStartup(MAKEWORD(2, 2), &wsData);
	if (reVal != 0)
	{
		std::cout << "WSAStartup error" << std::endl;
		return TCP_ERROR;
	}

	//����Socket
	Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == Socket)
	{
		std::cout << "Socket create error" << std::endl;
		return TCP_ERROR;
	}
		
	//����Socket������ģʽ
	unsigned long ul = 1;
	reVal = ioctlsocket(Socket, FIONBIO, (unsigned long*)&ul);
	if (SOCKET_ERROR == reVal)
	{
		std::cout << "Socket FIONBIO setting error" << std::endl;
		return TCP_ERROR;
	}
	
	//��Socket
	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(PortNumber);
	serAddr.sin_addr.S_un.S_addr = inet_addr(TCP_IPADDRESS);
	reVal = bind(Socket, (struct sockaddr*)&serAddr, sizeof(serAddr));
	if (SOCKET_ERROR == reVal)
	{
		std::cout << "Socket bind error" << std::endl;
		return TCP_ERROR;
	}

	//����
	reVal = listen(Socket, SOMAXCONN);
	if (SOCKET_ERROR == reVal)
	{
		std::cout << "Socket listen error" << std::endl;
		return TCP_ERROR;
	}
	
	Work = true;//���÷�����Ϊ����״̬

	//�����ͷ���Դ�߳�
	unsigned long ulThreadId;
	//�������տͻ��������߳�
	hAcceptThread = CreateThread(NULL, 0, AcceptThread, this, 0, &ulThreadId);
	if (NULL == hAcceptThread)
	{
		Work = FALSE;
		std::cout << "AcceptThread create error" << std::endl;
		return TCP_ERROR;
	}
	else
	{
		CloseHandle(hAcceptThread);
	}

	std::cout << "TCP Server succeeded!" << std::endl;
	std::cout << "Waiting for clients..." << std::endl;
	return TCP_OK;
}

/*
* ���������̣߳���Ѱ����Ŀͻ���
*/
DWORD __stdcall AcceptThread(void* pParam)
{
	SOCKET  sAccept;							//���ܿͻ������ӵ��׽���
	sockaddr_in addrClient;						//�ͻ���SOCKET��ַ
	time_t Time;
	TCPPACKAGE* TCPSERVER = (TCPPACKAGE*)pParam;
	while (TCPSERVER->Work)						           //��������״̬
	{
		memset(&addrClient, 0, sizeof(sockaddr_in));					//��ʼ��
		int	lenClient = sizeof(sockaddr_in);				        	//��ַ����
		sAccept = accept(TCPSERVER->Socket, (sockaddr*)&addrClient, &lenClient);	//���ܿͻ�����
		if (INVALID_SOCKET == sAccept)
		{
			Sleep(100);
			int nErrCode = WSAGetLastError();
			if (nErrCode == WSAEWOULDBLOCK)	//�޷��������һ�����赲���׽��ֲ���
			{
				Sleep(200);
				continue;//�����ȴ�
			}
			else
			{
				std::cout << "Server Accept Exit" << std::endl;
				return 0;//�߳��˳�
			}
		}
		else//���ܿͻ��˵�����
		{
			//��ʾ�ͻ��˵�IP�Ͷ˿�
			char* pClientIP = inet_ntoa(addrClient.sin_addr);
			u_short  clientPort = ntohs(addrClient.sin_port);
			std::cout << "Accept a client. IP: " << pClientIP << "\tPort: " << clientPort << std::endl;
			time(&Time);
			//��������
			struct CLIENTDATA TempClient;
			TempClient.IPAddress = pClientIP;
			TempClient.Socket = sAccept;
			TempClient.Time = Time;
			TCPSERVER->ClientDataList.push_back(TempClient);
		}
	}

	std::cout << "Server Accept Exit" << std::endl;
	return 0;//�߳��˳�
}



bool TCPPACKAGE::InitClient(void) //��ʼ���ͻ���
{
	//����ֵ
	int reVal;

	//��ʼ��Windows Sockets DLL
	WSADATA  wsData;
	reVal = WSAStartup(MAKEWORD(2, 2), &wsData);
	if (reVal != 0)
	{
		std::cout << "WSAStartup error" << std::endl;
		return TCP_ERROR;
	}

	//����Socket
	Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == Socket)
	{
		std::cout << "Socket create error" << std::endl;
		return TCP_ERROR;
	}

	//��Socket
	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(PortNumber);
	serAddr.sin_addr.S_un.S_addr = inet_addr(TCP_IPADDRESS);
	reVal = connect(Socket, (struct sockaddr*)&serAddr, sizeof(serAddr));
	if (SOCKET_ERROR == reVal)
	{
		std::cout << "Socket connect error" << std::endl;
		return TCP_ERROR;
	}

	//����Socket������ģʽ
	unsigned long ul = 1;
	reVal = ioctlsocket(Socket, FIONBIO, (unsigned long*)&ul);
	if (SOCKET_ERROR == reVal)
	{
		std::cout << "Socket FIONBIO setting error" << std::endl;
		return TCP_ERROR;
	}

	struct CLIENTDATA tempClientData;
	tempClientData.IPAddress = TCP_IPADDRESS;
	tempClientData.Socket = this->Socket;
	time(&tempClientData.Time);
	ClientDataList.push_back(tempClientData);
	std::cout << "Connect server success!!" << std::endl;
	return TCP_OK;
}











#define BadCountOut 5
void TCPPACKAGE::While(TCPDATALIST* Get, TCPDATALIST* Send)
{
	//��ʱ�ͻ���ά������
	CheckTimeoutClient();

	//�����յ�����
	int size = ClientDataList.size();
	int i;
	for (i = 0; i < size; i++)
	{
		unsigned char Data[MAXBUFFERDATA];
		unsigned char Length;
		if (recvData(ClientDataList[i].Socket, Data, &Length) == 1)
		{
			if (Length > 0)
			{
				unsigned char first = 0;
				while (first < Length)
				{
					struct TCPDATA tempData;
					first += StreamToData(&Data[first], Length - first, &tempData); 	//�����������
					tempData.IPAddress = ClientDataList[i].IPAddress;
					Get->push_back(tempData);
					//��������յ����ݵ�ʱ��
					time(&(ClientDataList[i].Time));
				}
			}
		}
	}

	int sendsize = Send->size();
	for (i = 0; i < sendsize; i++)
	{
		bool bad = true;
		//�Ӻ���ǰ������ͬip��ַ
		int index;
		for (index = size - 1; index >= 0; index--)
		{
			if (ClientDataList[index].IPAddress == (*Send)[i].IPAddress)
			{
				unsigned char Data[MAXBUFFERDATA];
				unsigned char Length = DataToStream(&((*Send)[i]), Data); //���뷢������
				if (sendData(ClientDataList[index].Socket, Data, Length) == TCP_OK) //�������͵�����
				{
					Send->erase(Send->begin() + i);
					i--;
					sendsize--;
					bad = false;
				}
				break;
			}
		}

		if (bad)
		{
			//δ�ҵ�IP��ַ��������+1
			(*Send)[i].BadCount++;

			//�������ﵽBadCountOut����Ϊ����Ч���ݣ�ɾ��
			if ((*Send)[i].BadCount > BadCountOut)
			{
				Send->erase(Send->begin() + i);
				i--;
				sendsize--;
			}
		}
	}
}

void TCPPACKAGE::CheckTimeoutClient(void)
{
	int i;
	int size = this->ClientDataList.size();
	time_t NowTime;
	time(&NowTime);

	for (i = 0; i < size; i++)
	{
		if (difftime(NowTime, this->ClientDataList[i].Time) > Timeout)
		{
			this->ClientDataList.erase(this->ClientDataList.begin() + i);
			std::cout << "Delect death client" << std::endl;
			return;
		}
	}

	if ((WorkMode == TCP_CLIENT) && (ClientDataList.size() == 0))
	{
		InitClient();
	}
}

unsigned char TCPPACKAGE::recvData(SOCKET s, unsigned char* buf, unsigned char* len)
{
	memset(buf, 0, MAXBUFFERDATA);		//��ս��ջ�����
	int	 nReadLen = 0;			//�����ֽ���
	bool bLineEnd = FALSE;		//�н���
	unsigned char retVal = 1;

	unsigned long ErrorCount = 0;

	while (!bLineEnd)
	{
		nReadLen = recv(s, (char *)buf, MAXBUFFERDATA, 0);
		if (SOCKET_ERROR == nReadLen)
		{
			int nErrCode = WSAGetLastError();
			if (WSAEWOULDBLOCK == nErrCode)	//�������ݻ�����������
			{
				ErrorCount++;
				if (ErrorCount > 1000)
				{
					retVal = 2;
					break;
				}
				continue;						//����ѭ��
			}
			else if (WSAENETDOWN == nErrCode || WSAETIMEDOUT == nErrCode || WSAECONNRESET == nErrCode) //�ͻ��˹ر�������
			{
				retVal = 0;	//������ʧ��
				break;							//�߳��˳�
			}
		}

		if (0 == nReadLen)           //δ��ȡ������
		{
			retVal = 2;
			break;
		}
		bLineEnd = TRUE;
		*len = (unsigned char)nReadLen;
	}
	return retVal;
}

bool TCPPACKAGE::sendData(SOCKET s, unsigned char* buf, unsigned char length)
{
	int retVal;                 //����ֵ
	bool bLineEnd = TRUE;		//�н���

	while (bLineEnd)
	{
		retVal = send(s, (char*)buf, length, 0);//һ�η���
		//������
		if (SOCKET_ERROR == retVal)
		{
			int nErrCode = WSAGetLastError();//�������
			if (WSAEWOULDBLOCK == nErrCode)
			{
				continue;
			}
			else if (WSAENETDOWN == nErrCode || WSAETIMEDOUT == nErrCode || WSAECONNRESET == nErrCode)
			{
				return TCP_ERROR;
			}
		}
		bLineEnd = FALSE;
	}

	//printf("Sent\r\n");
	return TCP_OK;		//���ͳɹ�
}

void TCPPACKAGE::DSInit(void)
{
	DTS.LastD0 = 0;
	DTS.Work = 0;
	DTS.totallength = 0;

	STD.LastD0 = 0;
	STD.Work = 0;
	STD.totallength = 0;
}

/*
* ����ʽ��
* ��ͷ + ���ݵ�ʵ�ʳ��� + ����
* E0 ��ͷ
* ��������E0����D0+DF��ʾ
* ��������D0����D0+CF��ʾ
*/
unsigned char TCPPACKAGE::StreamToData(unsigned char* inputbuf, unsigned char inputlength, TCPDATA* Output)
{
	int i;
	memset(Output->Data, 0, MAXBUFFERDATA);
	Output->length = 0;
	for (i = 0; i < inputlength; i++)
	{
		if (inputbuf[i] == 0xE0)
		{
			//��ͷ����ʼ��ȫ������
			STD.LastD0 = 0;
			STD.totallength = 0;
			STD.Work = 1;
		}
		else if (STD.Work)
		{
			if (STD.totallength == 0)
			{
				//����Ϊ0���������Ϊ����
				STD.totallength = inputbuf[i];
			}
			else
			{
				//���Ȳ�Ϊ0����ʼ��������
				if (STD.LastD0)
				{
					if (inputbuf[i] == 0xDF)
					{
						Output->Data[Output->length] = 0xE0;
						Output->length++;
					}
					else
					{
						Output->Data[Output->length] = 0xD0;
						Output->length++;
					}
					STD.LastD0 = 0;
				}
				else
				{
					if (inputbuf[i] == 0xD0)
					{
						STD.LastD0 = 1;
					}
					else
					{
						Output->Data[Output->length] = inputbuf[i];
						Output->length++;
					}
				}

				if (Output->length >= STD.totallength)
				{
					STD.Work = 0;
					return i+1;
				}
			}
		}
	}
	return i;
}

unsigned char TCPPACKAGE::DataToStream(TCPDATA* Input, unsigned char* outputbuf)
{
	memset(outputbuf, 0, MAXBUFFERDATA);
	int i;
	unsigned char length = 2;
	outputbuf[0] = 0xE0;
	outputbuf[1] = Input->length;
	for (i = 0; i < Input->length; i++)
	{
		switch (Input->Data[i])
		{
		case 0xE0:
			outputbuf[length] = 0xD0;
			length++;
			outputbuf[length] = 0xDF;
			length++;
			break;
		case 0xD0:
			outputbuf[length] = 0xD0;
			length++;
			outputbuf[length] = 0xCF;
			length++;
			break;
		default:
			outputbuf[length] = Input->Data[i];
			length++;
		}
	}

	return length;
}
