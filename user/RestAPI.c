 //RestAPI.c
#include "common.h" 
#include "RestAPI.h"
// #include "server.h"
#include "thingspeak.h"

#include "espmissingincludes.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"

#include "time_utils.h"

#include <string.h>

#include <gpio.h>

bool portsVal[NUM_OF_PORTS];

//PowerEvent _PowerEvents[MAX_POWER_EVENTS];
PowerEvent* PowerEvents;
PortInfo* ports;

//ip/flipinput/portnum - flips a given input (0-7)
//ip/open/portnum     - not working yet
//ip/status         - returns the ports status
//ip/events         - returns the events
//ip/event/ID(0-7)/ACTIVE(0-1)/PORT(0-7)/STARTTIME(HH:MM)/endtime(HH:MM)/REPEATEVERYDAY(1)  - sets a given event
//ip/time         - returns the time
//ip/portsinfo/   - ports names
//ip/portinfo/portname - 20chars      - sets a port name
//ip/initialize     - intialize to default
//ip/sntp           - show the sntp hours from GMT
//ip/sntp/hoursfromGMT    set hours from GMT  ( for israel should be 3)
//ip/getwifi/
// ip/scan

RestPtrs _RestPtrsTable[] = { 
  {"flipinput",&doFlipinput},		// ip/flipinput/portnum
  {"open", &doOpen},				// ip/open/portnum
  {"status", &doStatus},			// ip/status
  {"setwifi", &doSetWifi},			// ip/setwifi/
  {"getwifi", &doGetWifi},			// ip/getwifi/
  {"events", &doGetEvents},			// ip/events
  {"event", &doSetEvent},			// ip/event/ID(0-8)/ACTIVE(0-1)/PORT(0-8)/STARTTIME(HH:MM)/endtime(HH:MM)/REPEATEVERYDAY(1)
  {"time", &doGetTime},				// ip/time
  {"portsinfo", &doGetPorts},		// ip/portsinfo/
  {"portinfo", &doSetPorts},		// ip/portinfo/portname - 20chars
  {"initialize", &doInitialize},	// ip/initialize
  {"sntp", &doSNTP},				// ip/sntp       or ip/sntp/hoursfromGMT
  {"wifiport", &doWifiport},
  {"scan", &doScanWifi},
  {"setpin", &doSetPin},			// ip/pin/value    (0,1)
  {"END", &doStatus} // end of commands

};


void ICACHE_FLASH_ATTR InitializeRest()
{
	RestPtrsTable = _RestPtrsTable;
	PowerEvents =  &flashData->_PowerEvents[0];
	ports = &flashData->Ports[0];



	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);
	gpio16_output_conf();
	// PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPI16_U, FUNC_GPI16);

	// todo: loadevents from EEPROM
}

#ifdef ESP_1
uint32 portsBits[NUM_OF_PORTS] = {BIT2};
int PortPinNumber[NUM_OF_PORTS] = {2};
#else
uint32 portsBits[NUM_OF_PORTS] = {BIT0,BIT1,BIT12,BIT13,BIT14,BIT15,BIT16,BIT7};
int PortPinNumber[NUM_OF_PORTS] = {0,1,12,13,14,15,16,7};
#endif


bool ICACHE_FLASH_ATTR IsTimeInside(Time start, Time end, Time current)
{
	bool retVal = false;

	int startSec = start.Hour * 60 + start.Min;
	int endSec = end.Hour * 60 + end.Min;
	int currSec = current.Hour * 60 + current.Min;

	int diff = endSec - startSec;
	int diffCur = currSec - startSec;

	if (diff < 0 )
	{
		diff += 24*60;
	}
	if (diffCur < 0 )
	{
		diffCur += 24*60;
	}

	if (diff - diffCur > 0 )
	{
		retVal = true;
	}

	return retVal;
}
void ICACHE_FLASH_ATTR OneSecLoop()
{
	int i=0;
	PowerEvents =  &flashData->_PowerEvents[0];
	bool portsValtmp[NUM_OF_PORTS];

	os_printf("OneSecLoop ");
	os_printf("%s GMT%s%02d\" ",epoch_to_str(sntp_time+(sntp_tz*3600)),sntp_tz > 0 ? "+" : "",sntp_tz);

	DateTime now = Now();
	Time curr;
	curr.Hour = now.hour;
	curr.Min = now.min;

	for (i=0; i < NUM_OF_PORTS; i++)
	{
		portsValtmp[i] = portsVal[i];
	}

	for (i=0; i < MAX_TIMED_POWER_EVENTS; i++)
	{
		int inputNum = PowerEvents[i].Port;
		if (PowerEvents[i].Active != 0)
		{
			if (inputNum >= 0 && inputNum < NUM_OF_PORTS)
			{
				if (IsTimeInside(PowerEvents[i].StartTime, PowerEvents[i].EndTime, curr))
				{
					os_printf("Event On Occurred - %d, Start %d:%d End %d:%d\n",i, PowerEvents[i].StartTime.Hour, PowerEvents[i].StartTime.Min, PowerEvents[i].EndTime.Hour, PowerEvents[i].EndTime.Min);

					portsValtmp[inputNum] = 1;
				}
				else if (portsValtmp[inputNum] != 1) // there might be other events that set it to high
				{
					os_printf("Event Off Occurred - %d, Start %d:%d End %d:%d\n",i, PowerEvents[i].StartTime.Hour, PowerEvents[i].StartTime.Min, PowerEvents[i].EndTime.Hour, PowerEvents[i].EndTime.Min);
					portsValtmp[inputNum] = 0;
				}

			}
		}
	}

	for (i=0; i < NUM_OF_PORTS; i++)
	{
		if (portsVal[i] != portsValtmp[i])
		{
			if (portsValtmp[i] !=0)
			{
				// output pin on
				//Set GPIO2 to HIGH
				gpio_output_set(portsBits[i], 0, portsBits[i], 0);
				//GPIO_OUTPUT_SET
				portsVal[i] = 1;
				ThingSpeak();
			}
			else
			{
				// output pin off
				//Set GPIO2 to LOW
				gpio_output_set(0, portsBits[i], portsBits[i], 0);
				//GPIO_OUTPUT_SET
				portsVal[i] = 0;
				ThingSpeak();
			}
		}
	}
	os_printf(" - end\n");

}

void ICACHE_FLASH_ATTR doInitialize(ServerConnData* conn)
{
	int i, idx;
	char tmp[20];
	PortInfo* ports = &flashData->Ports[0];
	flashData->SNTP = 3;
	flashData->ServerPort = 80;

	// initialize ports
	for (i=0; i < NUM_OF_PORTS; i++)
	{

		os_sprintf(tmp, "Output%d", i);

		flashData->Ports[i].PortPinNumber = PortPinNumber[i];
		flashData->Ports[i].Type = PORT_OUTPUT;
		strncpy(&flashData->Ports[i].PortName[0], tmp, 20);
	}


	for (idx=0; idx<MAX_TIMED_POWER_EVENTS; idx++)
	{
		PowerEvents[idx].Active = 0;
		PowerEvents[idx].Port = 0;
		PowerEvents[idx].StartTime.Hour = 0;
		PowerEvents[idx].StartTime.Min = 0;
		PowerEvents[idx].EndTime.Hour = 0;
		PowerEvents[idx].EndTime.Min = 0;
	}

    flash_write();


}

void ICACHE_FLASH_ATTR doGetTime(ServerConnData* conn)
{
		char buff[50];
		char *p;
		int i=0;

		StartResponseJson(conn);

		httpdSend(conn,"{\"time\":{", -1);


//			os_sprintf(buff,"\"RL%d\":\"%d\"", i, portsVal[i], -1);
			os_sprintf(buff,"%s GMT%s%02d\"",epoch_to_str(sntp_time+(sntp_tz*3600)),sntp_tz > 0 ? "+" : "",sntp_tz);
			httpdSend(conn,buff, -1);

		httpdSend(conn,"}}", -1);

		xmitSendBuff(conn);
}

void ICACHE_FLASH_ATTR doGetPorts(ServerConnData* conn)
{
		char buff[50];
		char *p;
		int i=0;

		StartResponseJson(conn);

		httpdSend(conn,"{\"PortsInfo\":[", -1);
		for (i=0; i < NUM_OF_PORTS; i++)
		{
//			os_sprintf(buff,"\"RL%d\":\"%d\"", i, portsVal[i], -1);
			os_sprintf(buff,"{\"Name\":\"%s\", \"Type\":\"%d\", \"PinNum\":\"%d\"}", ports[i].PortName, ports[i].Type, ports[i].PortPinNumber);
			httpdSend(conn,buff, -1);
			if (i < NUM_OF_PORTS-1)
		   	{
				httpdSend(conn,",", -1);
		   	}

		}

		httpdSend(conn,"]}", -1);

		xmitSendBuff(conn);
}

void ICACHE_FLASH_ATTR doSetPorts(ServerConnData* conn)
{
	// port/id/name
	char buff[256];
	os_printf("doSetPort\r\n");
	char tmp[120];
	int idx;
	int i=2;

    getValue(tmp, conn->url,'/',i);
    os_printf("IDX = %s", tmp);
    idx = atoi(tmp);

    i++;
    getValue(tmp, conn->url,'/',i);
    os_printf("name = %s", tmp);
    strncpy(ports[idx].PortName, tmp, 20);

    flash_write();
    doGetPorts(conn);
}

void ICACHE_FLASH_ATTR doSNTP(ServerConnData* conn)
{
	// port/id/name
	char buff[10];
	os_printf("doSetPort\r\n");
	char tmp[10];
	int idx;
	int i=2;

    if (getValue(tmp, conn->url,'/',i)!= -1)
    {
		os_printf("SNTP = %s", tmp);
		idx = atoi(tmp);

    	flashData->SNTP = idx;
        flash_write();
    }


	StartResponseJson(conn);

	httpdSend(conn,"{\"sntp\":", -1);

	os_sprintf(buff,"\"%d\"", flashData->SNTP, -1);
	httpdSend(conn,buff, -1);
	httpdSend(conn,"}", -1);

	xmitSendBuff(conn);


}

void ICACHE_FLASH_ATTR SendPortStatus(ServerConnData* conn)
{

	char buff[200]; 
	char *p;
	int i=0;

	StartResponseJson(conn);

	httpdSend(conn,"{\"ports\":{", -1);

	for (i=0; i<NUM_OF_PORTS; i++)
	{
		os_sprintf(buff,"\"%s\":\"%d\"", &flashData->Ports[i].PortName[0], portsVal[i], -1);
		httpdSend(conn,buff, -1);
		if (i < NUM_OF_PORTS-1)
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

	httpdSend(conn,"{\"Events\":[", -1);

	for (i=0; i<MAX_TIMED_POWER_EVENTS; i++)
	{
		//(v.id , v.input, v.Start, v.End, v.Interval, v.Active);
		os_sprintf(buff,"{\"%s\":\"%d\",", "id", i, -1);
		httpdSend(conn,buff, -1);
		os_sprintf(buff,"\"%s\":\"%d\",", "Active", PowerEvents[i].Active, -1);
		httpdSend(conn,buff, -1);
		os_sprintf(buff,"\"%s\":\"%d\",", "input", PowerEvents[i].Port, -1);
		httpdSend(conn,buff, -1);
		os_sprintf(buff,"\"%s\":\"%d\",", "Interval", PowerEvents[i].DaysRepeat, -1);
		httpdSend(conn,buff, -1);
		os_sprintf(buff,"\"Start\":\"%2d:%2d\",", PowerEvents[i].StartTime.Hour, PowerEvents[i].StartTime.Min, -1);
		httpdSend(conn,buff, -1);
		os_sprintf(buff,"\"End\":\"%2d:%2d\"}", PowerEvents[i].EndTime.Hour, PowerEvents[i].EndTime.Min, -1);
		httpdSend(conn,buff, -1);

		if (i < MAX_TIMED_POWER_EVENTS-1)
	   	{
			httpdSend(conn,",", -1);
	   	}
	}
	httpdSend(conn,"]}", -1);

	xmitSendBuff(conn);

}

void ICACHE_FLASH_ATTR doSetEvent(ServerConnData* conn)
{	// TODO: need to add validation and error handeling

	// setevent/ID/ACTIVE/PORT/STARTTIME(HH:MM)/endtime(HH:MM)/REPEATEVERYDAY
	char buff[256];
	os_printf("doSetEvent\r\n");
	char tmp[120];
	char tmp2[10];
	int idx;
	int i=2;

    getValue(tmp, conn->url,'/',i);
    os_printf("IDX = %s", tmp);
    idx = atoi(tmp);

    i++;
    getValue(tmp, conn->url,'/',i);
    os_printf("Active = %s", tmp);
    PowerEvents[idx].Active = atoi(tmp);

    i++;
    getValue(tmp, conn->url,'/',i);
    os_printf("port = %s", tmp);
    PowerEvents[idx].Port = atoi(tmp);

	// get startTime
    i++;
    getValue(tmp, conn->url,'/',i);
    getValue(tmp2, tmp,':',0); // hours
    PowerEvents[idx].StartTime.Hour = atoi(tmp2);
    getValue(tmp2, tmp,':',1); // min
    PowerEvents[idx].StartTime.Min = atoi(tmp2);

    //get endTime
    i++;
    getValue(tmp, conn->url,'/',i);
    getValue(tmp2, tmp,':',0); // hours
    PowerEvents[idx].EndTime.Hour = atoi(tmp2);
    getValue(tmp2, tmp,':',1); // min
    PowerEvents[idx].EndTime.Min = atoi(tmp2);

    //get repeat
    i++;
    getValue(tmp, conn->url,'/',i);
    //PowerEvents[idx].DaysRepeat = (Days)atoi(tmp);

    flash_write();
    doGetEvents(conn);
}


void ICACHE_FLASH_ATTR doSetWifi(ServerConnData* conn)
{	// ip/setwifi/SSID/pass
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

void PortPinSet(int inputNum, bool inValue)
{

	if (PortPinNumber[inputNum] == 16)
	{
		gpio16_output_set(inValue);
	}
	else
	{

	    if (!inValue)
	    {
	        //Set GPIO to LOW
			os_printf("Pin %d = 0\r\n", inputNum);
		    gpio_output_set(0, portsBits[inputNum], portsBits[inputNum], 0);
	        portsVal[inputNum] = 0;
	        // GPIO_OUTPUT_SET(inputNum,0);
	    }
	    else
	    {
	        //Set GPIO to HIGH
			os_printf("Pin %d = 1\r\n", inputNum);
	        gpio_output_set(portsBits[inputNum], 0, portsBits[inputNum], 0);
	        portsVal[inputNum] = 1;
	        // GPIO_OUTPUT_SET(inputNum, 1);
	    }
	}

}


void ICACHE_FLASH_ATTR doSetPin(ServerConnData* conn)
{
	os_printf("doSetPin\r\n");

	char param[10];
	char param2[10];
    getValue(param, conn->url,'/',2);
    getValue(param2, conn->url,'/',3);
	int inputNum = atoi(param);
	int inputVal = atoi(param2);

	PortPinSet(inputNum, inputVal);
	
    SendPortStatus(conn);

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
        PortPinSet(inputNum, 0);
    }
    else
    {
        PortPinSet(inputNum, 1);
    }
    SendPortStatus(conn);
}


LOCAL os_timer_t open_timer;

void ICACHE_FLASH_ATTR setPortToLow(void* inputNum)
{

	PortPinSet((int)inputNum, 0);
	//Set GPIO to LOW
	os_printf("setPortToLow %d", (int)inputNum);
}

void ICACHE_FLASH_ATTR doOpen(ServerConnData* conn)
{
	os_printf("doOpen");

	char param[50];
	int inputNum = 6;

	int test = getValue(param, conn->url,'/',2);
	os_printf("url - %s       getValue = %d\r\n", conn->url, test);

    if (getValue(param, conn->url,'/',2) != -1)
    {

    	inputNum = atoi(param);
    }

	 //Set GPIO to HIGH
	PortPinSet(inputNum, 1);

	os_timer_disarm(&open_timer);
    os_timer_setfn(&open_timer, (os_timer_func_t *)setPortToLow, (void*)inputNum);
    os_timer_arm(&open_timer, 3000, 0);



	char buff[50]; 

	StartResponseJson(conn);

	os_sprintf(buff,"{\"Open\": %d }", inputNum);
	httpdSend(conn,buff, -1);

	xmitSendBuff(conn);


}

void ICACHE_FLASH_ATTR doWifiport(ServerConnData* conn)
{
	os_printf("doWifiport");
	// port/id/name
	char buff[10];

	char tmp[10];
	int idx;
	int i=2;

    if (getValue(tmp, conn->url,'/',i)!= -1)
    {
		os_printf("wifi port = %s", tmp);
		idx = atoi(tmp);

    	flashData->ServerPort = idx;
        flash_write();
    }


	StartResponseJson(conn);

	httpdSend(conn,"{\"ServerPort\":", -1);

	os_sprintf(buff,"\"%d\"", flashData->ServerPort, -1);
	httpdSend(conn,buff, -1);
	httpdSend(conn,"}", -1);

	xmitSendBuff(conn);



}

void ICACHE_FLASH_ATTR doStatus(ServerConnData* conn)
{
	os_printf("doStatus");
    SendPortStatus(conn);
	os_printf("doStatus Done!");
}


void ICACHE_FLASH_ATTR wifiscan_done_callback(void *arg, STATUS status)
{

  if (status == OK)
  {
    struct bss_info *bss = (struct bss_info*)arg;
    bss = STAILQ_NEXT(bss, next);

    while(bss)
    {
      os_printf("%s %d %d %d\n", bss->ssid, bss->channel, bss->rssi, bss->authmode);
      bss = STAILQ_NEXT(bss, next);
    }
  }
}

void ICACHE_FLASH_ATTR StartScanWifi()
{
    os_printf("Starting scanning...");
    if (wifi_station_scan(NULL, wifiscan_done_callback))
    {
        os_printf("OK!");

    }
    else
    {
        os_printf("Error...");
    }
}

void ICACHE_FLASH_ATTR doScanWifi(ServerConnData* conn)
{
 	char buff[50];
	char *p;
	int i=0;

	StartResponseJson(conn);

	httpdSend(conn,"{\"ports\":{", -1);

	for (i=0; i<NUM_OF_PORTS; i++)
	{
		os_sprintf(buff,"\"%s\":\"%d\"", &flashData->Ports[i].PortName[0], portsVal[i], -1);
		httpdSend(conn,buff, -1);
		if (i < NUM_OF_PORTS-1)
	   	{
			httpdSend(conn,",", -1);
	   	}
	}
	httpdSend(conn,"}}", -1);

	xmitSendBuff(conn);

}



