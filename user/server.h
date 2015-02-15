#ifndef SERVER_H
#define SERVER_H

#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>


void ICACHE_FLASH_ATTR ServerInit(int port);


typedef struct  {
	struct espconn *conn;
	char *url;
	int postLen;
	char *postBuff;
} ServerConnData;



#endif //SERVER_H
