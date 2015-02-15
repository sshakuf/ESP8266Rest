#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"

#include "user_interface.h"
#include "uart.h"

#include "c_types.h"
#include "espconn.h"
#include "mem.h"

#include "common.h"
#include "server.h"

#include <stdio.h>
#include <stdarg.h>

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);

static volatile os_timer_t some_timer;

void dbgprint1(char* inBuff)
{
    uart0_tx_buffer(inBuff,strlen(inBuff));
}

void dbgprint(const char* format, ... ) {
    va_list args;
    char buff[512];
    va_start( args, format );
    ets_sprintf( buff, format, args );
    va_end( args );
    uart0_tx_buffer(buff,strlen(buff));

}


void some_timerfunc(void *arg)
{
    //os_printf("Hello\n\r");
    //dbgprint("\r\nready\r\n");
    //Do blinky stuff
    if (GPIO_REG_READ(GPIO_OUT_ADDRESS) & BIT2)
    {
        //Set GPIO2 to LOW
        gpio_output_set(0, BIT2, BIT2, 0);
        // dbgprint("\r\nGPIO2 to LOW\r\n");
    }
    else
    {
        //Set GPIO2 to HIGH
        gpio_output_set(BIT2, 0, BIT2, 0);
        // dbgprint("\r\nGPIO2 to HIGH\r\n");
    }
}

//Main code function
static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
    os_printf("loop\n\r");
    os_delay_us(10000);
    system_os_post(user_procTaskPrio, 0, 0 );
}

static void ICACHE_FLASH_ATTR networkConnectedCb(void *arg);
static void ICACHE_FLASH_ATTR networkDisconCb(void *arg);
static void ICACHE_FLASH_ATTR networkReconCb(void *arg, sint8 err);
static void ICACHE_FLASH_ATTR networkRecvCb(void *arg, char *data, unsigned short len);
static void ICACHE_FLASH_ATTR networkSentCb(void *arg);
void ICACHE_FLASH_ATTR network_init();

LOCAL os_timer_t network_timer;

static void ICACHE_FLASH_ATTR networkServerFoundCb(const char *name, ip_addr_t *ip, void *arg) {
  static esp_tcp tcp;
  struct espconn *conn=(struct espconn *)arg;
  if (ip==NULL) {
    os_printf("Nslookup failed :/ Trying again...\n");
    uart0_tx_buffer("lfai",4);
    network_init();
  }

  uart0_tx_buffer("lokk",4);
  char page_buffer[20];
  os_sprintf(page_buffer,"DST: %d.%d.%d.%d",
  *((uint8 *)&ip->addr), *((uint8 *)&ip->addr + 1),
  *((uint8 *)&ip->addr + 2), *((uint8 *)&ip->addr + 3));
  uart0_tx_buffer(page_buffer,strlen(page_buffer));

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

  ServerInit(80);
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


void ICACHE_FLASH_ATTR network_start() {
  static struct espconn conn;
  static ip_addr_t ip;
  os_printf("Looking up server...\n");
    uart0_tx_buffer("look",4);
  espconn_gethostbyname(&conn, "www.google.com", &ip, networkServerFoundCb);
}

void ICACHE_FLASH_ATTR network_check_ip(void) {
  struct ip_info ipconfig;
  os_timer_disarm(&network_timer);
  wifi_get_ip_info(STATION_IF, &ipconfig);
  if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {
    char page_buffer[20];
    os_sprintf(page_buffer,"IP: %d.%d.%d.%d",IP2STR(&ipconfig.ip));
    uart0_tx_buffer(page_buffer,strlen(page_buffer));
    network_start();
  } else {
    os_printf("No ip found\n\r");
    os_timer_setfn(&network_timer, (os_timer_func_t *)network_check_ip, NULL);
    os_timer_arm(&network_timer, 1000, 0);
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
    os_timer_arm(&some_timer, 1000, 1);
    
}

//Init function 
void ICACHE_FLASH_ATTR user_init() {

    uart_init(BIT_RATE_115200,BIT_RATE_115200);
    char ssid[32] = SSID;
    char password[64] = SSID_PASSWORD;
    struct station_config stationConf;

    user_init_gpio();
    //Set station mode
    wifi_set_opmode( 0x1 );

    //Set ap settings
    os_memcpy(&stationConf.ssid, ssid, 32);
    os_memcpy(&stationConf.password, password, 64);
    wifi_station_set_config(&stationConf);

    uart0_tx_buffer("init",4);

    //Start os task
    system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);

    system_os_post(user_procTaskPrio, 0, 0 );

    network_init();
}



