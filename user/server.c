
#include "server.h"
#include "common.h"

#include "espmissingincludes.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"

static struct espconn httpdConn;
static esp_tcp httpdTcp;


//Max length of request head
#define MAX_HEAD_LEN 1024
//Max amount of connections
#define MAX_CONN 8
//Max post buffer len
#define MAX_POST 1024
//Max send buffer len
#define MAX_SENDBUFF_LEN 2048

static ServerConnData gServerConnData[MAX_CONN];


//Looks up the connData info for a specific esp connection
static ServerConnData ICACHE_FLASH_ATTR *httpdFindConnData(void *arg) {
	int i;
	for (i=0; i<MAX_CONN; i++) {
		if (gServerConnData[i].conn==(struct espconn *)arg) return &gServerConnData[i];
	}
	os_printf("FindConnData: Huh? Couldn't find connection for %p\n", arg);
	return NULL; //WtF?
}

//Retires a connection for re-use
static void ICACHE_FLASH_ATTR httpdRetireConn(ServerConnData *conn) {
	if (conn->postBuff!=NULL) os_free(conn->postBuff);
	conn->postBuff=NULL;
	conn->conn=NULL;
}


//Callback called when the data on a socket has been successfully
//sent.
static void ICACHE_FLASH_ATTR httpdSentCb(void *arg) {
	int r;
	// HttpdConnData *conn=httpdFindConnData(arg);
	// char sendBuff[MAX_SENDBUFF_LEN];

	dbgprint("Sent callback on conn \n");
	// if (conn==NULL) return;
	// conn->priv->sendBuff=sendBuff;
	// conn->priv->sendBuffLen=0;

	// if (conn->cgi==NULL) { //Marked for destruction?
	// 	os_printf("Conn %p is done. Closing.\n", conn->conn);
	// 	espconn_disconnect(conn->conn);
	// 	httpdRetireConn(conn);
	// 	return; //No need to call xmitSendBuff.
	// }

	// r=conn->cgi(conn); //Execute cgi fn.
	// if (r==HTTPD_CGI_DONE) {
	// 	conn->cgi=NULL; //mark for destruction.
	// }
	// xmitSendBuff(conn);
}



//Parse a line of header data and modify the connection data accordingly.
static void ICACHE_FLASH_ATTR ServerParseHeaderURL(char *h, ServerConnData* conn) {
	int i;
//	os_printf("Got header %s\n", h);
	// if (os_strncmp(h, "GET ", 4)==0 || os_strncmp(h, "POST ", 5)==0) {
	// 	char *e;
		
	// 	//Skip past the space after POST/GET
	// 	i=0;
	// 	while (h[i]!=' ') i++;
	// 	conn->url=h+i+1;

	// 	//Figure out end of url.
	// 	e=(char*)os_strstr(conn->url, " ");
	// 	if (e==NULL) return; //wtf?
	// 	*e=0; //terminate url part

	// 	os_printf("URL = %s\n", conn->url);
	// 	//Parse out the URL part before the GET parameters.
	// 	conn->getArgs=(char*)os_strstr(conn->url, "?");
	// 	if (conn->getArgs!=0) {
	// 		*conn->getArgs=0;
	// 		conn->getArgs++;
	// 		os_printf("GET args = %s\n", conn->getArgs);
	// 	} else {
	// 		conn->getArgs=NULL;
	// 	}
	// } else if (os_strncmp(h, "Content-Length: ", 16)==0) {
	// 	i=0;
	// 	//Skip trailing spaces
	// 	while (h[i]!=' ') i++;
	// 	//Get POST data length
	// 	int postLen=atoi(h+i+1);
	// 	//Clamp if too big. Hmm, maybe we should error out instead?
	// 	if (postLen>MAX_POST)
	// 		dbgprint("Mallocced buffer for xxx bytes of post data.\n");
	// }
}


static void ICACHE_FLASH_ATTR httpdRecvCb(void *arg, char *data, unsigned short len) {
	dbgprint(data);
	ServerConnData conn;
	// find GET

	//GET /test/ HTTP/1.1
}

static void ICACHE_FLASH_ATTR httpdReconCb(void *arg, sint8 err) {
	// HttpdConnData *conn=httpdFindConnData(arg);
	dbgprint("ReconCb\n");
	// if (conn==NULL) return;
	//Yeah... No idea what to do here. ToDo: figure something out.
}

static void ICACHE_FLASH_ATTR httpdDisconCb(void *arg) {

}


static void ICACHE_FLASH_ATTR httpdConnectCb(void *arg) {
	struct espconn *conn=arg;
	// ServerConnData serverConn;
	// serverConn->conn = conn;
	int i;

     dbgprint("\r\nconnection httpd\r\n");

	// os_printf("Con req, conn=%p, pool slot %d\n", conn, i);
	
	// //Find empty conndata in pool
	// for (i=0; i<MAX_CONN; i++) if (connData[i].conn==NULL) break;
	// os_printf("Con req, conn=%p, pool slot %d\n", conn, i);
	// if (i==MAX_CONN) {
	// 	os_printf("Aiee, conn pool overflow!\n");
	// 	espconn_disconnect(conn);
	// 	return;
	// }
	// connData[i].priv=&connPrivData[i];
	// connData[i].conn=conn;
	// connData[i].priv->headPos=0;
	// connData[i].postBuff=NULL;
	// connData[i].priv->postPos=0;
	// connData[i].postLen=-1;

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
	httpdConn.type=ESPCONN_TCP;
	httpdConn.state=ESPCONN_NONE;
	httpdTcp.local_port=port;
	httpdConn.proto.tcp=&httpdTcp;

	os_printf("Httpd init, conn=%p\n", &httpdConn);
	espconn_regist_connectcb(&httpdConn, httpdConnectCb);
	espconn_accept(&httpdConn);
}


