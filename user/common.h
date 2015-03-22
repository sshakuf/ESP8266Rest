#ifndef COMMON_H
#define COMMON_H

#include "c_types.h"

//#define SSID "STRS"
//#define SSID_PASSWORD "1011040311037"
#define SSID "CheggBackupHot"
#define SSID_PASSWORD "Ch399!M3"
//#define SSID "klar_wifi"
//#define SSID_PASSWORD "idonoa2013"

#define AP_SSID "ESP"
#define AP_PASSWORD "12341234"

// void dbgprint1(char* inBuff);
// void dbgprint(const char* format, ... );
// char dbgbuff[512];
// #define dbgprintf(f_, ...) os_sprintf(dbgbuff, (f_), __VA_ARGS__);\
//     			dbgprint(dbgbuff);



typedef struct
{
	int Hour;
	int Min;
} Time;

typedef struct
{
    int Sunday : 1;
    int Monday    : 1;
    int Tuesday       : 1;
    int Wednesday   : 1;
    int Thursday   : 1;
    int Friday   : 1;
    int Saturday   : 1;
} Days;

typedef struct
{
	Time StartTime;
	Time EndTime;
	Days DaysRepeat;
	int Port;
}PowerEvent;

#define MAX_TIMED_POWER_EVENTS 10

//__declspec(align(4))  // needs to align ? 
typedef union _DWORD_PART_ {
    char settings[128];

    struct {
        int magic;
        char ssid[32];
        char password[64];
        PowerEvent _PowerEvents[MAX_TIMED_POWER_EVENTS];
    } ;
} FlashData;

#define MAGIC_NUM 7059
#define PRIV_PARAM_START_SEC 0x3c
#define PRIV_PARAM_SAVE     0

FlashData* flashData;
char* IPStation;

bool ICACHE_FLASH_ATTR IsStationConnected();

#endif //COMMON_H
