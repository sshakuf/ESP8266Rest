#ifndef COMMON_H
#define COMMON_H


// #define SSID "STRS"
// #define SSID_PASSWORD "1011040311037"
#define SSID "CheggBackupHot"
#define SSID_PASSWORD "Ch399!M3"  
#define AP_SSID "ESP"
#define AP_PASSWORD "12341234"

// void dbgprint1(char* inBuff);
// void dbgprint(const char* format, ... );
// char dbgbuff[512];
// #define dbgprintf(f_, ...) os_sprintf(dbgbuff, (f_), __VA_ARGS__);\
//     			dbgprint(dbgbuff);

typedef union _DWORD_PART_ {
    char settings[128];

    struct {
        int magic;
        char ssid[32];
        char password[64];
    } ;
} FlashData;

#define MAGIC_NUM 7059
#define PRIV_PARAM_START_SEC 0x3c
#define PRIV_PARAM_SAVE     0

FlashData* flashData;
char* IPStation;

#endif //COMMON_H
