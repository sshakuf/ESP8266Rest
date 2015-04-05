/*
 * thingspeak.h
 *
 *  Created on: Apr 1, 2015
 *      Author: sshakuf
 */

#ifndef USER_THINGSPEAK_H_
#define USER_THINGSPEAK_H_

#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>
#include "server.h"

#include "common.h"

void ICACHE_FLASH_ATTR ThingSpeak();

void ICACHE_FLASH_ATTR OpenURL(char* inURL);
static void ICACHE_FLASH_ATTR URLRecivedCb(const char *name, ip_addr_t *ip, void *arg);
static void ICACHE_FLASH_ATTR URLnetworkConnectedCb(void *arg);
static void ICACHE_FLASH_ATTR URLnetworkRecvCb(void *arg, char *data, unsigned short len);
static void ICACHE_FLASH_ATTR URLnetworkSentCb(void *arg);


#endif /* USER_THINGSPEAK_H_ */
