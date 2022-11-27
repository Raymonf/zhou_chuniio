#include "TCPserver.h"
#include <time.h>
#pragma comment(lib,"ws2_32.lib")  //静态加载ws2_32.lib

DWORD __stdcall AcceptThread(void* pParam);

bool TCPPACKAGE::Init(bool mode, int port, int timeoutsec)
{
	this->WorkMode = mode;
	this->PortNumber = port;
	this->Timeout = timeoutsec;
	sendFirst = true;
	DSInit(); //初始化翻译标志位

	//根据模式初始化相应操作
	if (mode == TCP_SERVER)
		return InitServer();
	else
		return InitClient();
}


bool TCPPACKAGE::InitServer(void) //初始化服务器
{
	//返回值
	int reVal;

	//初始化Windows Sockets DLL
	WSADATA  wsData;
	reVal = WSAStartup(MAKEWORD(2, 2), &wsData);
	if (reVal != 0)
	{
		std::cout << "WSAStartup error" << std::endl;
		return TCP_ERROR;
	}

	//创建Socket
	Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == Socket)
	{
		std::cout << "Socket create error" << std::endl;
		return TCP_ERROR;
	}
		
	//设置Socket非阻塞模式
	unsigned long ul = 1;
	reVal = ioctlsocket(Socket, FIONBIO, (unsigned long*)&ul);
	if (SOCKET_ERROR == reVal)
	{
		std::cout << "Socket FIONBIO setting error" << std::endl;
		return TCP_ERROR;
	}
	
	//绑定Socket
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

	//监听
	reVal = listen(Socket, SOMAXCONN);
	if (SOCKET_ERROR == reVal)
	{
		std::cout << "Socket listen error" << std::endl;
		return TCP_ERROR;
	}
	
	Work = true;//设置服务器为运行状态

	//创建释放资源线程
	unsigned long ulThreadId;
	//创建接收客户端请求线程
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
* 服务器多线程：搜寻接入的客户端
*/
DWORD __stdcall AcceptThread(void* pParam)
{
	SOCKET  sAccept;							//接受客户端连接的套接字
	sockaddr_in addrClient;						//客户端SOCKET地址
	time_t Time;
	TCPPACKAGE* TCPSERVER = (TCPPACKAGE*)pParam;
	while (TCPSERVER->Work)						           //服务器的状态
	{
		memset(&addrClient, 0, sizeof(sockaddr_in));					//初始化
		int	lenClient = sizeof(sockaddr_in);				        	//地址长度
		sAccept = accept(TCPSERVER->Socket, (sockaddr*)&addrClient, &lenClient);	//接受客户请求
		if (INVALID_SOCKET == sAccept)
		{
			Sleep(100);
			int nErrCode = WSAGetLastError();
			if (nErrCode == WSAEWOULDBLOCK)	//无法立即完成一个非阻挡性套接字操作
			{
				Sleep(200);
				continue;//继续等待
			}
			else
			{
				std::cout << "Server Accept Exit" << std::endl;
				return 0;//线程退出
			}
		}
		else//接受客户端的请求
		{
			//显示客户端的IP和端口
			char* pClientIP = inet_ntoa(addrClient.sin_addr);
			u_short  clientPort = ntohs(addrClient.sin_port);
			std::cout << "Accept a client. IP: " << pClientIP << "\tPort: " << clientPort << std::endl;
			time(&Time);
			//加入容器
			struct CLIENTDATA TempClient;
			TempClient.IPAddress = pClientIP;
			TempClient.Socket = sAccept;
			TempClient.Time = Time;
			TCPSERVER->ClientDataList.push_back(TempClient);
		}
	}

	std::cout << "Server Accept Exit" << std::endl;
	return 0;//线程退出
}



bool TCPPACKAGE::InitClient(void) //初始化客户端
{
	//返回值
	int reVal;

	//初始化Windows Sockets DLL
	WSADATA  wsData;
	reVal = WSAStartup(MAKEWORD(2, 2), &wsData);
	if (reVal != 0)
	{
		std::cout << "WSAStartup error" << std::endl;
		return TCP_ERROR;
	}

	//创建Socket
	Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == Socket)
	{
		std::cout << "Socket create error" << std::endl;
		return TCP_ERROR;
	}

	//绑定Socket
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

	//设置Socket非阻塞模式
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
	//超时客户端维护程序
	CheckTimeoutClient();

	//检查接收的内容
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
					first += StreamToData(&Data[first], Length - first, &tempData); 	//翻译接收内容
					tempData.IPAddress = ClientDataList[i].IPAddress;
					Get->push_back(tempData);
					//更新最后收到数据的时间
					time(&(ClientDataList[i].Time));
				}
			}
		}
	}

	int sendsize = Send->size();
	for (i = 0; i < sendsize; i++)
	{
		bool bad = true;
		//从后往前查找相同ip地址
		int index;
		for (index = size - 1; index >= 0; index--)
		{
			if (ClientDataList[index].IPAddress == (*Send)[i].IPAddress)
			{
				unsigned char Data[MAXBUFFERDATA];
				unsigned char Length = DataToStream(&((*Send)[i]), Data); //翻译发送内容
				if (sendData(ClientDataList[index].Socket, Data, Length) == TCP_OK) //丢出发送的内容
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
			//未找到IP地址，坏计数+1
			(*Send)[i].BadCount++;

			//坏计数达到BadCountOut则认为是无效数据，删除
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
	memset(buf, 0, MAXBUFFERDATA);		//清空接收缓冲区
	int	 nReadLen = 0;			//读入字节数
	bool bLineEnd = FALSE;		//行结束
	unsigned char retVal = 1;

	unsigned long ErrorCount = 0;

	while (!bLineEnd)
	{
		nReadLen = recv(s, (char *)buf, MAXBUFFERDATA, 0);
		if (SOCKET_ERROR == nReadLen)
		{
			int nErrCode = WSAGetLastError();
			if (WSAEWOULDBLOCK == nErrCode)	//接受数据缓冲区不可用
			{
				ErrorCount++;
				if (ErrorCount > 1000)
				{
					retVal = 2;
					break;
				}
				continue;						//继续循环
			}
			else if (WSAENETDOWN == nErrCode || WSAETIMEDOUT == nErrCode || WSAECONNRESET == nErrCode) //客户端关闭了连接
			{
				retVal = 0;	//读数据失败
				break;							//线程退出
			}
		}

		if (0 == nReadLen)           //未读取到数据
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
	int retVal;                 //返回值
	bool bLineEnd = TRUE;		//行结束

	while (bLineEnd)
	{
		retVal = send(s, (char*)buf, length, 0);//一次发送
		//错误处理
		if (SOCKET_ERROR == retVal)
		{
			int nErrCode = WSAGetLastError();//错误代码
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
	return TCP_OK;		//发送成功
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
* 流格式：
* 包头 + 数据的实际长度 + 数据
* E0 包头
* 数据中有E0，用D0+DF表示
* 数据中有D0，用D0+CF表示
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
			//包头，初始化全部内容
			STD.LastD0 = 0;
			STD.totallength = 0;
			STD.Work = 1;
		}
		else if (STD.Work)
		{
			if (STD.totallength == 0)
			{
				//长度为0，则此数据为长度
				STD.totallength = inputbuf[i];
			}
			else
			{
				//长度不为0，开始翻译数据
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
