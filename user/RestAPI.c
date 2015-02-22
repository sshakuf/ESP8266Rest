//RestAPI.c
#include "common.h"
#include "RestAPI.h"
// #include "server.h"

#include "espmissingincludes.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"

#include <string.h>

RestPtrs _RestPtrsTable[] = { 
  {"flipinput",&doFlipinput},
  {"open", &doOpen},
  {"status", &doStatus},
  {"setwifi", &doSetWifi},
  {"getwifi", &doGetWifi}
};

void ICACHE_FLASH_ATTR InitializeRest()
{
	RestPtrsTable = _RestPtrsTable;
}

uint32 portsBits[NUM_OF_OUTPUT_PORTS] = {BIT5,BIT0};

void ICACHE_FLASH_ATTR SendPortStatus(ServerConnData* conn)
{

	char buff[50];
	char *p;
	int i=0;

	StartResponse(conn, 200);
	AddHeader(conn, "Content-Type", "application/json");
	AddHeader(conn, "Access-Control-Allow-Origin", "*");
	AddHeader(conn, "Access-Control-Allow-Methods", "GET, POST, PUT");
	AddHeader(conn, "Connection", "close");
	EndHeaders(conn);

	httpdSend(conn,"{\"ports\":{", -1);

	for (i=0; i<NUM_OF_OUTPUT_PORTS; i++)
	{
		os_sprintf(buff,"\"RL%d\":\"%d\"", i, portsVal[i], -1);
		httpdSend(conn,buff, -1);
		if (i < NUM_OF_OUTPUT_PORTS-1)
	   	{
			httpdSend(conn,",", -1);
	   	}
	}
	httpdSend(conn,"}}", -1);

	xmitSendBuff(conn);

}

void ICACHE_FLASH_ATTR doSetWifi(ServerConnData* conn)
{
	char buff[256];
	os_printf("doSetWifi\r\n");
	char paramSSID[120];
	char paramPASS[120];
    getValue(paramSSID, conn->url,'/',2);
    getValue(paramPASS, conn->url,'/',3);

	os_printf("paramSSID 2= %s\r\n", paramSSID);
	os_printf("paramPASS 2= %s\r\n", paramPASS);
	int passsize = strlen(paramPASS);
	int x= passsize < 3 ? passsize/2 : 3;
	paramPASS[x] = '\0';

    os_memcpy(flashData->ssid, paramSSID, 32);
    os_memcpy(flashData->password, paramPASS, 64);
	os_sprintf(buff,"{'ssid':'%s', 'pass':'%s...'}", flashData->ssid, paramPASS);

	flashData->magic = MAGIC_NUM;
    
	flash_write();
    SendHTTPResponse(conn, buff);


}
void ICACHE_FLASH_ATTR doGetWifi(ServerConnData* conn)
{
	char buff[256];
	os_printf("doGetWifi\r\n");

	flash_read();
	os_printf("SSID 2= %s\r\n", flashData->ssid);
	//os_printf("PASS 2= %s\r\n", flashData->password);

	os_sprintf(buff,"{'ssid':'%s', 'pass':'xxxx', 'IP':'%s'}", flashData->ssid, IPStation);
    SendHTTPResponse(conn, buff);

}
void ICACHE_FLASH_ATTR doFlipinput(ServerConnData* conn)
{
	os_printf("doFlipinput\r\n");

	char param[20];
    getValue(param, conn->url,'/',2);
	int inputNum = atoi(param);

	os_printf("param 2= %d\r\n", inputNum);


    if (GPIO_REG_READ(GPIO_OUT_ADDRESS) & portsBits[inputNum])
    {
        //Set GPIO2 to LOW
        gpio_output_set(0, portsBits[inputNum], portsBits[inputNum], 0);
        portsVal[inputNum] = 0;
    }
    else
    {
        //Set GPIO2 to HIGH
        gpio_output_set(portsBits[inputNum], 0, portsBits[inputNum], 0);
        portsVal[inputNum] = 1;
    }
    SendPortStatus(conn);
}
void ICACHE_FLASH_ATTR doOpen(ServerConnData* conn)
{
	os_printf("doOpen");
}
void ICACHE_FLASH_ATTR doStatus(ServerConnData* conn)
{
	os_printf("doStatus");
    SendPortStatus(conn);
}

