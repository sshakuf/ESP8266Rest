#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "osapi.h"
#include "user_config.h"
#include "stdout.h"

#include "user_interface.h"
#include "uart.h"

#include "c_types.h"
#include "espconn.h"
#include "mem.h"

#include "common.h"
#include "server.h" 

#include <stdio.h>
#include <stdarg.h>

#include "sntp.h"   

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);

static volatile os_timer_t some_timer;

// void dbgprint1(char* inBuff)
// {
//     uart0_tx_buffer(inBuff,strlen(inBuff));
// }

// void dbgprint(const char* format, ... ) {
//     va_list args;
//     char buff[512];
//     va_start( args, format );
//     os_sprintf(buff, format, args );
//     uart0_tx_buffer(buff,strlen(buff));
//     va_end( args );
// }

#include "thingspeak.h"



void some_timerfunc(void *arg)
{
    if (GPIO_REG_READ(GPIO_OUT_ADDRESS) & BIT2)
    {
        //Set GPIO2 to LOW
        gpio_output_set(0, BIT2, BIT2, 0);
    }
    else
    {
        //Set GPIO2 to HIGH
        gpio_output_set(BIT2, 0, BIT2, 0);
    }
}

uint32 stime=0;
int gSec=0;

//Main code function
static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
	struct tm *info;
	char buffer[80];

	uint32 tmp = 0;
    //os_printf("loop\n\r");
    os_delay_us(10000);

    tmp = system_get_time();
    if (tmp - stime > 5000000) // 5 sec has passed
    {
        stime = tmp;
    	gSec++;
    	//os_printf("Sec=%d\n", gSec);

//    	  char tmp[100];
//    	  os_sprintf(tmp,"Time: %s GMT%s%02d\n",epoch_to_str(sntp_time+(sntp_tz*3600)),sntp_tz > 0 ? "+" : "",sntp_tz);
//    	  os_printf("%s\n",tmp);
    	OneSecLoop();

    }



    system_os_post(user_procTaskPrio, 0, 0 );
}

bool ICACHE_FLASH_ATTR IsStationConnected()
{
	  struct ip_info ipconfig;

	  wifi_get_ip_info(STATION_IF, &ipconfig);
	  if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {
		  return true;
	  }
	  return false;
}

static void ICACHE_FLASH_ATTR networkConnectedCb(void *arg);
static void ICACHE_FLASH_ATTR networkDisconCb(void *arg);
static void ICACHE_FLASH_ATTR networkReconCb(void *arg, sint8 err);
static void ICACHE_FLASH_ATTR networkRecvCb(void *arg, char *data, unsigned short len);
static void ICACHE_FLASH_ATTR networkSentCb(void *arg);
void ICACHE_FLASH_ATTR network_init();

LOCAL os_timer_t network_timer;

static void ICACHE_FLASH_ATTR networkTimeFoundCb(const char *name, ip_addr_t *ip, void *arg) {
  static esp_tcp tcp;
  struct espconn *conn=(struct espconn *)arg;
  if (ip==NULL) {
    os_printf("Nslookup failed :/ Trying again...\n");
    os_printf("lfai");
    network_init();
  }

  // os_printf("lokk"); 
  char page_buffer[20];
  os_sprintf(page_buffer,"DST: %d.%d.%d.%d",
  *((uint8 *)&ip->addr), *((uint8 *)&ip->addr + 1),
  *((uint8 *)&ip->addr + 2), *((uint8 *)&ip->addr + 3));
  os_printf(page_buffer);  

  // conn->type=ESPCONN_TCP;
  // conn->state=ESPCONN_NONE;
  // conn->proto.tcp=&tcp;
  // conn->proto.tcp->local_port=espconn_port();
  // conn->proto.tcp->remote_port=80;
  // os_memcpy(conn->proto.tcp->remote_ip, &ip->addr, 4);
  // espconn_regist_connectcb(conn, networkConnectedCb);
  // espconn_regist_disconcb(conn, networkDisconCb);
  // espconn_regist_reconcb(conn, networkReconCb);
  // espconn_regist_recvcb(conn, networkRecvCb);
  // espconn_regist_sentcb(conn, networkSentCb);
  // espconn_connect(conn);

}

static void ICACHE_FLASH_ATTR networkSentCb(void *arg) {
  uart0_tx_buffer("sent",4);
}

static void ICACHE_FLASH_ATTR networkRecvCb(void *arg, char *data, unsigned short len) {

  uart0_tx_buffer("recv",4);
  
  struct espconn *conn=(struct espconn *)arg;
  int x;
  uart0_tx_buffer(data,len);
  //for (x=0; x<len; x++) networkParseChar(conn, data[x]);
}

static void ICACHE_FLASH_ATTR networkConnectedCb(void *arg) {

  uart0_tx_buffer("conn",4);
  struct espconn *conn=(struct espconn *)arg;

  // char *data = "GET / HTTP/1.0\r\n\r\n\r\n";
  // sint8 d = espconn_sent(conn,data,strlen(data));

  // espconn_regist_recvcb(conn, networkRecvCb);
  // uart0_tx_buffer("cend",4);
}

static void ICACHE_FLASH_ATTR networkReconCb(void *arg, sint8 err) {
  uart0_tx_buffer("rcon",4);
//  os_printf("Reconnect\n\r");
//  network_init();
}

static void ICACHE_FLASH_ATTR networkDisconCb(void *arg) {
  uart0_tx_buffer("dcon",4);
//  os_printf("Disconnect\n\r");
//  network_init();
}


void ICACHE_FLASH_ATTR GetNetworkTime() {
  static struct espconn conn;
  static ip_addr_t ip;
  os_printf("Looking up server...\n");
    os_printf("look");
  espconn_gethostbyname(&conn, "www.google.com", &ip, networkTimeFoundCb);
}

void ICACHE_FLASH_ATTR SetSetverMode()
{
    struct softap_config apConfig;

    os_memcpy(&apConfig.ssid, AP_SSID,32);
    os_memcpy(&apConfig.password, AP_PASSWORD,32);
    apConfig.ssid_len = strlen(AP_SSID);
    apConfig.channel = 6;
    apConfig.authmode = AUTH_WPA_PSK;   
    wifi_softap_set_config(&apConfig);
    wifi_softap_dhcps_start();

}

int counter = 0;
char ipstation[20];

void ICACHE_FLASH_ATTR network_check_ip(void) {
  struct ip_info ipconfig;
  os_timer_disarm(&network_timer);
  wifi_get_ip_info(STATION_IF, &ipconfig);
  if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {
    char page_buffer[20];
    os_sprintf(page_buffer,"IP: %d.%d.%d.%d",IP2STR(&ipconfig.ip));
    os_sprintf(ipstation, "%d.%d.%d.%d",IP2STR(&ipconfig.ip));
    os_printf(page_buffer);

    ServerInit(flashData->ServerPort);


    //GetNetworkTime();
    os_printf("SNTP-----%d", flashData->SNTP);
    sntp_init(flashData->SNTP);

  } else {
    counter++;
    os_printf("No ip found\n\r");
    if (counter < 12)
    {
        os_timer_setfn(&network_timer, (os_timer_func_t *)network_check_ip, NULL);
        os_printf("try %d\n", counter);
        os_timer_arm(&network_timer, 1000, 0);
    }
    else
    {
        os_printf("could not connect to server\n", counter);
        wifi_station_disconnect();
        ServerInit(flashData->ServerPort);

        counter = 0;
    }
  }
}

void ICACHE_FLASH_ATTR network_init() {
  os_timer_disarm(&network_timer);
  os_timer_setfn(&network_timer, (os_timer_func_t *)network_check_ip, NULL);
  os_timer_arm(&network_timer, 1000, 0);
}

void ICACHE_FLASH_ATTR
user_init_gpio()
{

    // Initialize the GPIO subsystem.
    gpio_init();

    //Set GPIO2 to output mode
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);

    //Set GPIO2 low
    gpio_output_set(0, BIT2, BIT2, 0);

    //Disarm timer
    os_timer_disarm(&some_timer);

    //Setup timer
    os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);

    //Arm the timer
    //&some_timer is the pointer
    //1000 is the fire time in ms
    //0 for once and 1 for repeating
    //os_timer_arm(&some_timer, 1000, 1);
    
} 


FlashData _flashData;

void ICACHE_FLASH_ATTR initFlash()
{	// demo data for new devices.
	flashData->magic = MAGIC_NUM;

	PortInfo* ports = &flashData->Ports[0];

	flashData->Ports[0].PortPinNumber = 5;
	flashData->Ports[0].Type = PORT_OUTPUT;
	strncpy(&flashData->Ports[0].PortName[0], "output1", 20);

	flashData->Ports[1].PortPinNumber = 0;
	flashData->Ports[1].Type = PORT_OUTPUT;
	strncpy(&flashData->Ports[1].PortName[0], "output2", 20);

	flashData->SNTP = 3;
	flashData->ServerPort = 80;
}

void ICACHE_FLASH_ATTR flash_write() {
    os_printf("flashWrite() size-%d\n", sizeof(FlashData));
    ETS_UART_INTR_DISABLE();

    spi_flash_erase_sector(PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE);
    spi_flash_write(0x3c000, (uint32 *)flashData, sizeof(FlashData));

    ETS_UART_INTR_ENABLE();

  // spi_flash_read(0x3C000, (uint32 *) settings, 1024);
 
  // spi_flash_erase_sector(0x3C);
  // spi_flash_write(0x3C000,(uint32 *)settings,1024);
 
}
 
void ICACHE_FLASH_ATTR flash_read() {
    os_printf("flashRead() size-%d\n", sizeof(FlashData));

    flashData->magic =0;
    ETS_UART_INTR_DISABLE();
    spi_flash_read((PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE) * SPI_FLASH_SEC_SIZE,
                (uint32 *)flashData, sizeof(FlashData));
    ETS_UART_INTR_ENABLE();
    if (flashData->magic != MAGIC_NUM)
    {
    	os_printf("ReadFlash ERROR!\n");
    	initFlash();
    }
}

//Init function 
void ICACHE_FLASH_ATTR user_init() {

    //uart_init(BIT_RATE_115200,BIT_RATE_115200);
    flashData = &_flashData;
    IPStation = ipstation;

    stdoutInit();
    char ssid[32] = SSID;
    char password[64] = SSID_PASSWORD;
    struct station_config stationConf;


    os_printf("\ninit\n");


    user_init_gpio();
    //Set station mode & AP mode
    // wifi_set_opmode(STATION_MODE);
    wifi_set_opmode(STATIONAP_MODE);

    initFlash();
    flash_read();

    //if (flashData->magic != MAGIC_NUM)
    {
    //Set ap settings
        os_memcpy(&stationConf.ssid, ssid, 32);
        os_memcpy(&stationConf.password, password, 64);
    }
//    else
//    {
//        os_memcpy(&stationConf.ssid, flashData->ssid, 32);
//        os_memcpy(&stationConf.password, flashData->password, 64);
//
//    }
    wifi_station_set_config(&stationConf);
    os_printf("\nConnecting to %s, %s\n", stationConf.ssid, stationConf.password);

    SetSetverMode();


    //Start os task
    system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);

    system_os_post(user_procTaskPrio, 0, 0 );

    os_printf("\nInitializing Network\n");
    network_init(); 


}



