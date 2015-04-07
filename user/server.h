#ifndef SERVER_H
#define SERVER_H

#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>


void ICACHE_FLASH_ATTR ServerInit(int port);

typedef struct HttpdPriv HttpdPriv;

typedef struct  {
	struct espconn *conn;
	char *url;
	int postLen;
	char *postBuff;
	HttpdPriv *priv;
	char *getArgs;
} ServerConnData;

void ICACHE_FLASH_ATTR SendHTTPResponse(ServerConnData* conn, char* msg);
void ICACHE_FLASH_ATTR SendPortStatus(ServerConnData* conn);
void ICACHE_FLASH_ATTR ParseURLCommand(char *h, ServerConnData* conn);
ServerConnData ICACHE_FLASH_ATTR *httpdFindConnData(void *arg);
static void ICACHE_FLASH_ATTR ServerRetireConn(ServerConnData *conn);
int ICACHE_FLASH_ATTR httpdSend(ServerConnData *conn, const char *data, int len);
void ICACHE_FLASH_ATTR xmitSendBuff(ServerConnData *conn);
static void ICACHE_FLASH_ATTR ServerParseHeaderURL(char *h, ServerConnData* conn);
void ICACHE_FLASH_ATTR StartResponse(ServerConnData *conn, int code);
void ICACHE_FLASH_ATTR AddHeader(ServerConnData *conn, const char *field, const char *val);
void ICACHE_FLASH_ATTR EndHeaders(ServerConnData *conn);
void ICACHE_FLASH_ATTR StartResponseJson(ServerConnData *conn);
int ICACHE_FLASH_ATTR getValue(char* retParam, const char* data, char separator, int index);



#endif //SERVER_H
