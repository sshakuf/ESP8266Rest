//RestAPI.h
#ifndef RESTAPI_H
#define RESTAPI_H

#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>
#include "server.h"

#include "common.h"

extern bool portsVal[NUM_OF_PORTS];

typedef struct 
{
  char* command;
  void(*f)(ServerConnData* conn);
} RestPtrs;

void ICACHE_FLASH_ATTR OneSecLoop();


void ICACHE_FLASH_ATTR InitializeRest();
void ICACHE_FLASH_ATTR SendPortStatus(ServerConnData* conn);
void ICACHE_FLASH_ATTR doSetWifi(ServerConnData* conn);
void ICACHE_FLASH_ATTR doGetWifi(ServerConnData* conn);
void ICACHE_FLASH_ATTR doFlipinput(ServerConnData* conn);
void ICACHE_FLASH_ATTR doOpen(ServerConnData* conn);
void ICACHE_FLASH_ATTR doStatus(ServerConnData* conn);
void ICACHE_FLASH_ATTR doGetEvents(ServerConnData* conn);
void ICACHE_FLASH_ATTR doSetEvent(ServerConnData* conn);
void ICACHE_FLASH_ATTR doGetTime(ServerConnData* conn);
void ICACHE_FLASH_ATTR doGetPorts(ServerConnData* conn);
void ICACHE_FLASH_ATTR doSetPorts(ServerConnData* conn);
void ICACHE_FLASH_ATTR doInitialize(ServerConnData* conn);
void ICACHE_FLASH_ATTR doSNTP(ServerConnData* conn);
void ICACHE_FLASH_ATTR doWifiport(ServerConnData* conn);
void ICACHE_FLASH_ATTR doScanWifi(ServerConnData* conn);
void ICACHE_FLASH_ATTR doSetPin(ServerConnData* conn);



RestPtrs* RestPtrsTable;


#endif //RESTAPI_H
