
#include "server.h"
#include "common.h"

#include "espmissingincludes.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"

#include <string.h>

static struct espconn gEspConn;
static esp_tcp httpdTcp;


//Max length of request head
#define MAX_HEAD_LEN 1024
//Max amount of connections
#define MAX_CONN 8
//Max post buffer len
#define MAX_POST 1024
//Max send buffer len
#define MAX_SENDBUFF_LEN 2048

struct HttpdPriv {
	char head[MAX_HEAD_LEN];
	int headPos;
	int postPos;
	char *sendBuff;
	int sendBuffLen;
};

//Connection pool
static HttpdPriv connPrivData[MAX_CONN];
static ServerConnData gServerConnData[MAX_CONN];

typedef struct 
{
  char* command;
  void(*f)(ServerConnData* conn);
} RestPtrs;

#define NUM_OF_OUTPUT_PORTS 2
bool portsVal[NUM_OF_OUTPUT_PORTS];
uint32 portsBits[NUM_OF_OUTPUT_PORTS] = {BIT2,BIT0};

static void ICACHE_FLASH_ATTR SendPortStatus(ServerConnData* conn)
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

static void ICACHE_FLASH_ATTR doFlipinput(ServerConnData* conn)
{
	dbgprint("doFlipinput\r\n");

	char param[20];
    getValue(param, conn->url,'/',2);
	int inputNum = atoi(param);

	dbgprintf("param 2= %d\r\n", inputNum);


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
static void ICACHE_FLASH_ATTR doOpen(ServerConnData* conn)
{
	dbgprint("doOpen");
}
static void ICACHE_FLASH_ATTR doStatus(ServerConnData* conn)
{
	dbgprint("doStatus");
    SendPortStatus(conn);
}

RestPtrs RestPtrsTable[] = { 
  {"flipinput",&doFlipinput},
  {"open", &doOpen},
  {"status", &doStatus},
};
#define NUMOFCOMMANDS 3

static void ICACHE_FLASH_ATTR getValue(char* retParam, const char* data, char separator, int index)
{
  int found = 0;
  int i=0;
  int strIndex[] = {0, -1  };
  int maxIndex = strlen(data)-1;
  for(i=0; i<=maxIndex && found<=index; i++){
	  if(data[i]==separator || i==maxIndex){
	  found++;
	  strIndex[0] = strIndex[1]+1;
	  strIndex[1] = (i == maxIndex) ? i : i;
	  }
  }
  int size = strIndex[1]-strIndex[0];
  dbgprintf("\nget param 0=%d,1=%d\n", strIndex[0], strIndex[1])
  strncpy(retParam, &data[strIndex[0]],(size_t)size );
  retParam[strIndex[1]-1] = '\0';
}


static void ICACHE_FLASH_ATTR ParseURLCommand(char *h, ServerConnData* conn) {
	char* pb = conn->url;
	int idx;
	char param[20];
	// if (strncmp(pb, "GET /", 5) == 0) {
 //        pb +=5;
        
        getValue(param, conn->url,'/',1);
        dbgprint("param - ");
        dbgprint(param);
        bool handeled = false;
        for (idx = 0; idx < NUMOFCOMMANDS; idx++)
        {
          if (strcmp(param,RestPtrsTable[idx].command) == 0)
          {
            RestPtrsTable[idx].f(conn);
            handeled = true;
            break;
          }
        }
        if (!handeled)
        {
          // dbgTerminalprintln("NO REST REPLAY");
        }
      // }
}


//Looks up the connData info for a specific esp connection
static ServerConnData ICACHE_FLASH_ATTR *httpdFindConnData(void *arg) {
	int i;
	for (i=0; i<MAX_CONN; i++) {
		if (gServerConnData[i].conn==(struct espconn *)arg) return &gServerConnData[i];
	}
	dbgprintf("FindConnData: Huh? Couldn't find connection for %p\n", arg);
	return NULL; //WtF?
}

//Retires a connection for re-use
static void ICACHE_FLASH_ATTR ServerRetireConn(ServerConnData *conn) {
	if (conn->postBuff!=NULL) os_free(conn->postBuff);
	conn->postBuff=NULL;
	conn->conn=NULL;
}

//Start the response headers.
void ICACHE_FLASH_ATTR StartResponse(ServerConnData *conn, int code) {
	char buff[128];
	int l;
	l=os_sprintf(buff, "HTTP/1.0 %d OK\r\nServer: esp8266-httpd\"HTTPDVER\"\r\n", code);
	httpdSend(conn, buff, l);
}

//Send a http header.
void ICACHE_FLASH_ATTR AddHeader(ServerConnData *conn, const char *field, const char *val) {
	char buff[256];
	int l;

	l=os_sprintf(buff, "%s: %s\r\n", field, val);
	httpdSend(conn, buff, l);
}

//Finish the headers.
void ICACHE_FLASH_ATTR EndHeaders(ServerConnData *conn) {
	httpdSend(conn, "\r\n", -1);
}

//Add data to the send buffer. len is the length of the data. If len is -1
//the data is seen as a C-string.
//Returns 1 for success, 0 for out-of-memory.
int ICACHE_FLASH_ATTR httpdSend(ServerConnData *conn, const char *data, int len) {
	if (len<0) len=strlen(data);
	if (conn->priv->sendBuffLen+len>MAX_SENDBUFF_LEN) return 0;
	os_memcpy(conn->priv->sendBuff+conn->priv->sendBuffLen, data, len);
	conn->priv->sendBuffLen+=len;
	return 1;
}

//Helper function to send any data in conn->priv->sendBuff
static void ICACHE_FLASH_ATTR xmitSendBuff(ServerConnData *conn) {
	if (conn->priv->sendBuffLen!=0) {
		espconn_sent(conn->conn, (uint8_t*)conn->priv->sendBuff, conn->priv->sendBuffLen);
		conn->priv->sendBuffLen=0;
	}
}

//Callback called when the data on a socket has been successfully
//sent.
static void ICACHE_FLASH_ATTR httpdSentCb(void *arg) {
	int r;
	ServerConnData *conn=httpdFindConnData(arg);
	char sendBuff[MAX_SENDBUFF_LEN];

	dbgprint("Sent callback on conn \n");
	if (conn==NULL) return;
	conn->priv->sendBuff=sendBuff;
	conn->priv->sendBuffLen=0;

	// if (conn->cgi==NULL) { //Marked for destruction?
		dbgprint("Conn %p is done. Closing.\n", conn->conn);
		espconn_disconnect(conn->conn);
		ServerRetireConn(conn);
		return; //No need to call xmitSendBuff.
	// }

	// r=conn->cgi(conn); //Execute cgi fn.
	// if (r==HTTPD_CGI_DONE) {
	// 	conn->cgi=NULL; //mark for destruction.
	// }
	// xmitSendBuff(conn);
}


static const char *httpNotFoundHeader="HTTP/1.0 404 Not Found\r\nServer: esp8266-httpd/0.1\r\nContent-Type: text/plain\r\n\r\nNot Found.\r\n";

//This is called when the headers have been received and the connection is ready to send
//the result headers and data.
static void ICACHE_FLASH_ATTR httpdSendResp(ServerConnData *conn) {
	int i=0;
	int r;
	//See if the url is somewhere in our internal url table.
// 	while (builtInUrls[i].url!=NULL && conn->url!=NULL) {
// 		int match=0;
// //		os_printf("%s == %s?\n", builtInUrls[i].url, conn->url);
// 		if (os_strcmp(builtInUrls[i].url, conn->url)==0) match=1;
// 		if (builtInUrls[i].url[os_strlen(builtInUrls[i].url)-1]=='*' &&
// 				os_strncmp(builtInUrls[i].url, conn->url, os_strlen(builtInUrls[i].url)-1)==0) match=1;
// 		if (match) {
// 			os_printf("Is url index %d\n", i);
// 			conn->cgiData=NULL;
// 			conn->cgi=builtInUrls[i].cgiCb;
// 			conn->cgiArg=builtInUrls[i].cgiArg;
// 			r=conn->cgi(conn);
// 			if (r!=HTTPD_CGI_NOTFOUND) {
// 				if (r==HTTPD_CGI_DONE) conn->cgi=NULL;  //If cgi finishes immediately: mark conn for destruction.
// 				return;
// 			}
// 		}
// 		i++;
// 	}
// 	//Can't find :/
	os_printf("%s not found. 404!\n", conn->url);
	// httpdSend(conn, httpNotFoundHeader, -1);
	// conn->cgi=NULL; //mark for destruction
}

//Parse a line of header data and modify the connection data accordingly.
static void ICACHE_FLASH_ATTR ServerParseHeaderURL(char *h, ServerConnData* conn) {
	int i;

	if (os_strncmp(h, "GET ", 4)==0 || os_strncmp(h, "POST ", 5)==0) {
		char *e;
		
		//Skip past the space after POST/GET
		i=0;
		while (h[i]!=' ') i++;
		conn->url=h+i+1;

		//Figure out end of url.
		e=(char*)os_strstr(conn->url, " ");
		if (e==NULL) return; //wtf?
		*e=0; //terminate url part

		dbgprintf("URL = %s\n", conn->url);
		//Parse out the URL part before the GET parameters.
		conn->getArgs=(char*)os_strstr(conn->url, "?");
		if (conn->getArgs!=0) {
			*conn->getArgs=0;
			conn->getArgs++;
			dbgprintf("GET args = %s\n", conn->getArgs); 

		} else {
			conn->getArgs=NULL;
		}
		ParseURLCommand(h,conn);

	} else if (os_strncmp(h, "Content-Length: ", 16)==0) {
		i=0;
		//Skip trailing spaces
		while (h[i]!=' ') i++;
		//Get POST data length
		int postLen=atoi(h+i+1);
		//Clamp if too big. Hmm, maybe we should error out instead?
		if (postLen>MAX_POST)
			dbgprint("Mallocced buffer for xxx bytes of post data.\n");
	}
}


static void ICACHE_FLASH_ATTR httpdRecvCb(void *arg, char *data, unsigned short len) {
	dbgprint("httpdRecvCb\r\n");
	//dbgprint(data);
	//ServerConnData conn;

	int x;
	char *p, *e;
	char sendBuff[MAX_SENDBUFF_LEN];
	ServerConnData *conn=httpdFindConnData(arg);
	if (conn==NULL) return;
	conn->priv->sendBuff=sendBuff;
	conn->priv->sendBuffLen=0;

	for (x=0; x<len; x++) {
		if (conn->postLen<0) {
			//This byte is a header byte.
			if (conn->priv->headPos!=MAX_HEAD_LEN) conn->priv->head[conn->priv->headPos++]=data[x];
			conn->priv->head[conn->priv->headPos]=0;
			//Scan for /r/n/r/n
			if (data[x]=='\n' && (char *)os_strstr(conn->priv->head, "\r\n\r\n")!=NULL) {
				//Indicate we're done with the headers.
				conn->postLen=0;
				//Reset url data
				conn->url=NULL;
				//Find end of next header line
				p=conn->priv->head;
				while(p<(&conn->priv->head[conn->priv->headPos-4])) {
					e=(char *)os_strstr(p, "\r\n");
					if (e==NULL) break;
					e[0]=0;
					ServerParseHeaderURL(p, conn);
					p=e+2;
				}
				//If we don't need to receive post data, we can send the response now.
				if (conn->postLen==0) {
					httpdSendResp(conn);
				}
			}
		} else if (conn->priv->postPos!=-1 && conn->postLen!=0 && conn->priv->postPos <= conn->postLen) {
			//This byte is a POST byte.
			conn->postBuff[conn->priv->postPos++]=data[x];
			if (conn->priv->postPos>=conn->postLen) {
				//Received post stuff.
				conn->postBuff[conn->priv->postPos]=0; //zero-terminate
				conn->priv->postPos=-1;
				os_printf("Post data: %s\n", conn->postBuff);
				//Send the response.
				httpdSendResp(conn);
				break;
			}
		}
	}
	// xmitSendBuff(conn);

}

static void ICACHE_FLASH_ATTR httpdReconCb(void *arg, sint8 err) {
	// HttpdConnData *conn=httpdFindConnData(arg);
	dbgprint("ReconCb\n");
	// if (conn==NULL) return;
	//Yeah... No idea what to do here. ToDo: figure something out.
}

static void ICACHE_FLASH_ATTR httpdDisconCb(void *arg) {
#if 0
	//Stupid esp sdk passes through wrong arg here, namely the one of the *listening* socket.
	//If it ever gets fixed, be sure to update the code in this snippet; it's probably out-of-date.
	HttpdConnData *conn=httpdFindConnData(arg);
	os_printf("Disconnected, conn=%p\n", conn);
	if (conn==NULL) return;
	conn->conn=NULL;
	if (conn->cgi!=NULL) conn->cgi(conn); //flush cgi data
#endif
	//Just look at all the sockets and kill the slot if needed.
	int i;
	for (i=0; i<MAX_CONN; i++) {
		if (gServerConnData[i].conn!=NULL) {
			//Why the >=ESPCONN_CLOSE and not ==? Well, seems the stack sometimes de-allocates
			//espconns under our noses, especially when connections are interrupted. The memory
			//is then used for something else, and we can use that to capture *most* of the
			//disconnect cases.
			if (gServerConnData[i].conn->state==ESPCONN_NONE || gServerConnData[i].conn->state>=ESPCONN_CLOSE) {
				gServerConnData[i].conn=NULL;
			}
		}
	}
}


static void ICACHE_FLASH_ATTR httpdConnectCb(void *arg) {
	struct espconn *conn=arg;
	int i;

     dbgprint("\r\nconnection httpd\r\n");

	dbgprintf("Con req, conn=%p, pool slot %d\n", conn, i);
	
	//Find empty conndata in pool
	for (i=0; i<MAX_CONN; i++) if (gServerConnData[i].conn==NULL) break;
	dbgprintf("Con req, conn=%p, pool slot %d\n", conn, i);
	if (i==MAX_CONN) {
		dbgprint("Aiee, conn pool overflow!\n");
		espconn_disconnect(conn);
		return;
	}
	gServerConnData[i].priv=&connPrivData[i];
	gServerConnData[i].conn=conn;
	gServerConnData[i].priv->headPos=0;
	gServerConnData[i].postBuff=NULL;
	gServerConnData[i].priv->postPos=0;
	gServerConnData[i].postLen=-1;

	espconn_regist_recvcb(conn, httpdRecvCb);
	espconn_regist_reconcb(conn, httpdReconCb);
	espconn_regist_disconcb(conn, httpdDisconCb);
	espconn_regist_sentcb(conn, httpdSentCb);
}

void ICACHE_FLASH_ATTR ServerInit(int port) {
	int i;

	// for (i=0; i<MAX_CONN; i++) {
	// 	connData[i].conn=NULL;
	// }
	gEspConn.type=ESPCONN_TCP;
	gEspConn.state=ESPCONN_NONE;
	httpdTcp.local_port=port;
	gEspConn.proto.tcp=&httpdTcp;

	dbgprintf("\nServer initiated, conn=%p\n", &gEspConn);
	espconn_regist_connectcb(&gEspConn, httpdConnectCb);
	espconn_accept(&gEspConn);
	dbgprintf("listening on port=%d\n", port);
}


