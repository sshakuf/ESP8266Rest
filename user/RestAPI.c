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


//PowerEvent _PowerEvents[MAX_POWER_EVENTS];
PowerEvent* PowerEvents;

RestPtrs _RestPtrsTable[] = { 
  {"flipinput",&doFlipinput},
  {"open", &doOpen},
  {"status", &doStatus},
  {"setwifi", &doSetWifi},
  {"getwifi", &doGetWifi},
  {"events", &doGetEvents},
  {"event", &doSetEvent},

  {"END", &doStatus} // end of commands

};

void ICACHE_FLASH_ATTR InitializeRest()
{
	RestPtrsTable = _RestPtrsTable;
	PowerEvents =  flashData->_PowerEvents;

	// todo: loadevents from EEPROM
}

uint32 portsBits[NUM_OF_OUTPUT_PORTS] = {BIT5,BIT0};

void ICACHE_FLASH_ATTR SendPortStatus(ServerConnData* conn)
{

	char buff[50];
	char *p;
	int i=0;

	StartResponseJson(conn);

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

void ICACHE_FLASH_ATTR doGetEvents(ServerConnData* conn)
{
	char buff[200];
	char *p;
	int i=0;

	StartResponseJson(conn);

	httpdSend(conn,"{\"Events\":{", -1);

	for (i=0; i<MAX_POWER_EVENTS; i++)
	{
		os_sprintf(buff,"\"Start\":\"%d:%d\"", PowerEvents[i].StartTime.Hour, PowerEvents[i].StartTime.Min, -1);
		httpdSend(conn,buff, -1);
		os_sprintf(buff,"\"End\":\"%d:%d\"", PowerEvents[i].EndTime.Hour, PowerEvents[i].EndTime.Min, -1);
		httpdSend(conn,buff, -1);

		if (i < NUM_OF_OUTPUT_PORTS-1)
	   	{
			httpdSend(conn,",", -1);
	   	}
	}
	httpdSend(conn,"}}", -1);

	xmitSendBuff(conn);

}

void ICACHE_FLASH_ATTR doSetEvent(ServerConnData* conn)
{
	// getevent/ID/STARTTIME(HH:MM)/endtime(HH:MM)/REPEATEVERYDAY
	char buff[256];
	os_printf("doSetEvent\r\n");
	char tmp[120];
	char tmp2[10];
	int idx;

    getValue(tmp, conn->url,'/',2);
    os_printf("IDX = %s", tmp);
    idx = atoi(tmp);

	// get startTime
    getValue(tmp, conn->url,'/',3);
    getValue(tmp2, tmp,':',0); // hours
    PowerEvents[idx].StartTime.Hour = atoi(tmp2);
    getValue(tmp2, tmp,':',1); // min
    PowerEvents[idx].StartTime.Min = atoi(tmp2);

    //get endTime
    getValue(tmp, conn->url,'/',4);
    getValue(tmp2, tmp,':',0); // hours
    PowerEvents[idx].EndTime.Hour = atoi(tmp2);
    getValue(tmp2, tmp,':',1); // min
    PowerEvents[idx].EndTime.Min = atoi(tmp2);

    //get repeat
    getValue(tmp, conn->url,'/',5);
    //PowerEvents[idx].DaysRepeat = (Days)atoi(tmp);

    flash_write();
    doGetEvents(conn);
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

