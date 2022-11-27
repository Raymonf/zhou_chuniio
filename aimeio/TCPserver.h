#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <vector>
#include <string.h>
#include <winsock2.h>

#define TCP_SERVER 0
#define TCP_CLIENT 1
#define TCP_ERROR 1
#define TCP_OK 0

#define MAXBUFFERDATA 128

#define TCP_IPADDRESS "127.0.0.1"

struct CLIENTDATA
{
	SOCKET Socket;
	std::string IPAddress;
	time_t Time;
};

struct TCPDATA
{
	unsigned char Data[MAXBUFFERDATA];
	unsigned char length;
	unsigned char BadCount;
	std::string IPAddress;
};
typedef std::vector<struct TCPDATA> TCPDATALIST;

class TCPPACKAGE
{
public:
	bool Init(bool mode, int port, int timeoutsec);
	void While(TCPDATALIST* Get, TCPDATALIST* Send);
	bool Work;
	SOCKET	Socket;
	HANDLE	hAcceptThread;
	HANDLE	hRecvThread;
	std::vector <struct CLIENTDATA> ClientDataList;

private:
	bool WorkMode;
	int PortNumber;
	int Timeout;
	
	bool InitServer(void);
	bool InitClient(void);

	void CheckTimeoutClient(void);
	unsigned char recvData(SOCKET s, unsigned char* buf, unsigned char* len);
	bool sendData(SOCKET s, unsigned char* buf, unsigned char length);

	struct DSINFO
	{
		bool Work;
		bool LastD0;
		unsigned char totallength;
	};
	struct DSINFO STD;
	struct DSINFO DTS;
	void DSInit(void);
	unsigned char StreamToData(unsigned char* inputbuf, unsigned char inputlength, TCPDATA* Output);
	unsigned char DataToStream(TCPDATA* Input, unsigned char* outputbuf);

	bool sendFirst;
};
