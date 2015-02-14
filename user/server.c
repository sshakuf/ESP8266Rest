
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




static void ICACHE_FLASH_ATTR httpdConnectCb(void *arg) {
	struct espconn *conn=arg;
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

	// espconn_regist_recvcb(conn, httpdRecvCb);
	// espconn_regist_reconcb(conn, httpdReconCb);
	// espconn_regist_disconcb(conn, httpdDisconCb);
	// espconn_regist_sentcb(conn, httpdSentCb);
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


