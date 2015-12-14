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

// scan fow wifi networkd 
// #define SCANNER 

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

void ICACHE_FLASH_ATTR network_init();

LOCAL os_timer_t network_timer;


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

void ICACHE_FLASH_ATTR initmDNS(struct ip_info ipconfig) {

  struct mdns_info *info = (struct mdns_info *)os_zalloc(sizeof(struct mdns_info));
  info->host_name = "espressif";
  info->ipAddr = ipconfig.ip.addr; //ESP8266 station IP
  info->server_name = "iot";
  info->server_port = 80;
  info->txt_data[0] = "version = now";
  // info->txt_data[1] = "user1 = data1";
  // info->txt_data[2] = "user2 = data2";
  espconn_mdns_init(info);
  // espconn_mdns_server_register(); 

}

int counter = 0;
char ipstation[20];
    struct station_config stationConf;

void ICACHE_FLASH_ATTR network_check_ip(void) {
  struct ip_info ipconfig;
  os_timer_disarm(&network_timer);
  wifi_get_ip_info(STATION_IF, &ipconfig);
  char buffer[20];
  
  os_printf("\nConnecting to %s... ", stationConf.ssid);
  os_sprintf(buffer,"State: %d - ",wifi_station_get_connect_status());
  os_printf(buffer);

  if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {
    char page_buffer[20];
    os_sprintf(page_buffer,"IP: %d.%d.%d.%d",IP2STR(&ipconfig.ip));
    os_sprintf(ipstation, "%d.%d.%d.%d",IP2STR(&ipconfig.ip));
    os_printf(page_buffer);

    ServerInit(flashData->ServerPort);


    os_printf("mDNS init");
    initmDNS(ipconfig);
    os_printf("mDNS end");

    //GetNetworkTime();
    os_printf("SNTP-----%d", flashData->SNTP);
    sntp_init(flashData->SNTP);

    // check network status in 10 min
    os_timer_arm(&network_timer, 10*60*1000, 0);// 10 min

  } else {
    counter++;
    os_printf("No ip found\n\r");
    if (counter < 30)
    {
        os_timer_setfn(&network_timer, (os_timer_func_t *)network_check_ip, NULL);
        os_printf("try %d\n", counter);
        os_timer_arm(&network_timer, 1000, 0);
    }
    else
    {
        os_printf("could not connect to server...\n", counter);
        wifi_station_disconnect();
        ServerInit(flashData->ServerPort);


        // doing reconnect
        os_timer_arm(&network_timer, 2*60*1000, 0);// 2 min
        wifi_station_connect();
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
  
void ICACHE_FLASH_ATTR ReadFromFlash() {
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
      os_printf("Flash Initialized!\n");
      flash_write();
      os_printf("Flash Write finished.!\n");
    }
    if (flashData->ServerPort == 0)
      {flashData->ServerPort=80;}
}


void ICACHE_FLASH_ATTR scan_done_callback(void *arg, STATUS status)
{

  if (status == OK)
  {
    struct bss_info *bss = (struct bss_info*)arg;
    bss = STAILQ_NEXT(bss, next);

    while(bss)
    {
      os_printf("%s %d %d %d\n", bss->ssid, bss->channel, bss->rssi, bss->authmode);
      bss = STAILQ_NEXT(bss, next);
    }
  }
}

void ICACHE_FLASH_ATTR init_done()
{
    os_printf("Starting scanning...");
    if (wifi_station_scan(NULL, scan_done_callback))
    {
        os_printf("OK!");

    }
    else
    {
        os_printf("Error..."); 
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
    //struct station_config stationConf;


    os_printf("\ninit\n");


    user_init_gpio();

     //Set station mode & AP mode
    // wifi_set_opmode(STATION_MODE);
    wifi_set_opmode(STATIONAP_MODE);

    initFlash();
    ReadFromFlash();

    //if (flashData->magic != MAGIC_NUM) 
    {
    //Set ap settings
    	stationConf.bssid_set = 0;
        os_memcpy(&stationConf.ssid, ssid, strlen(ssid));
        os_memcpy(&stationConf.password, password, strlen(password));
        if (flashData->ServerPort < 0)
        {
        	flashData->ServerPort = 80;
        }
    }
//    else 
//    {
//        os_memcpy(&stationConf.ssid, flashData->ssid, 32);
//        os_memcpy(&stationConf.password, flashData->password, 64);
//
//    }
    wifi_station_set_hostname(HOSTNAME);
    wifi_station_set_config(&stationConf);
    os_printf("\nConnecting to %s, %s\n", stationConf.ssid, stationConf.password);


#ifdef SCANNER
    wifi_station_set_auto_connect(FALSE);
    system_init_done_cb(init_done);
#else

   wifi_station_set_auto_connect(TRUE);

   SetSetverMode();

   //Start os task
   system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);

   system_os_post(user_procTaskPrio, 0, 0 );

   os_printf("\nInitializing Network\n");
   network_init();
#endif

}



