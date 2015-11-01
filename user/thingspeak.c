#include "thingspeak.h"
#include "RestAPI.h"

#include "espmissingincludes.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"


#include <string.h>

void ICACHE_FLASH_ATTR ThingSpeak() {
//	OpenURL("api.thingspeak.com");
}


void ICACHE_FLASH_ATTR OpenURL(char* inURL) {
  static struct espconn conn;
  static ip_addr_t ip;
  os_printf("Looking up server...\n");
  os_printf("look");
  espconn_gethostbyname(&conn, inURL, &ip, URLRecivedCb);
}

static void ICACHE_FLASH_ATTR URLRecivedCb(const char *name, ip_addr_t *ip, void *arg) {
  static esp_tcp tcp;
  struct espconn *conn=(struct espconn *)arg;
  if (ip==NULL) {
    os_printf("ERROR OPEN URL Trying again...\n");
  }

  // os_printf("lokk");
  char page_buffer[20];
  os_sprintf(page_buffer,"DST: %d.%d.%d.%d",
  *((uint8 *)&ip->addr), *((uint8 *)&ip->addr + 1),
  *((uint8 *)&ip->addr + 2), *((uint8 *)&ip->addr + 3));
  os_printf(page_buffer);

   conn->type=ESPCONN_TCP;
   conn->state=ESPCONN_NONE;
   conn->proto.tcp=&tcp;
   conn->proto.tcp->local_port=espconn_port();
   conn->proto.tcp->remote_port=80;
   os_memcpy(conn->proto.tcp->remote_ip, &ip->addr, 4);
   espconn_regist_connectcb(conn, URLnetworkConnectedCb);
//   espconn_regist_disconcb(conn, networkDisconCb);
//   espconn_regist_reconcb(conn, networkReconCb);
   espconn_regist_recvcb(conn, URLnetworkRecvCb);
   espconn_regist_sentcb(conn, URLnetworkSentCb);
   espconn_connect(conn);

}

static void ICACHE_FLASH_ATTR URLnetworkSentCb(void *arg) {
  uart0_tx_buffer("sent",4);
}

static void ICACHE_FLASH_ATTR URLnetworkConnectedCb(void *arg) {

	os_printf("\nconn\n",4);
  struct espconn *conn=(struct espconn *)arg;

//   char *data = "GET / HTTP/1.0\r\n\r\n\r\n";
  	  char data[100];
  	  os_sprintf(data, "GET /update?key=H3R64J7EHCJSNXT4&field1=%d HTTP/1.1\r\nHost: api.thingspeak.com\r\nCache-Control: no-cache\r\n\r\n\r\n", portsVal[0]);
//  	  char* data = "GET /update?key=5ZFIUM7ZZE8RITMD&field1=%d HTTP/1.1\r\nHost: api.thingspeak.com\r\nCache-Control: no-cache\r\n\r\n\r\n";
  	  os_printf("data - %s", data);
      //URI += addr;
   sint8 d = espconn_sent(conn,data,strlen(data));

   espconn_regist_recvcb(conn, URLnetworkRecvCb);
   os_printf("\ncend\n",4);
}
static void ICACHE_FLASH_ATTR URLnetworkRecvCb(void *arg, char *data, unsigned short len) {

	os_printf("\nrecv\n",4);

  struct espconn *conn=(struct espconn *)arg;
  int x;
  os_printf(data,len);
  //for (x=0; x<len; x++) networkParseChar(conn, data[x]);
}
