#ifndef COMMON_H
#define COMMON_H


#define SSID "STRS"
#define SSID_PASSWORD "1011040311037"
// #define SSID "CheggBackupHot"
// #define SSID_PASSWORD "Ch399!M3"
#define AP_SSID "ESP"
#define AP_PASSWORD "12341234"

void dbgprint1(char* inBuff);
void dbgprint(const char* format, ... );

char dbgbuff[512];

#define dbgprintf(f_, ...) os_sprintf(dbgbuff, (f_), __VA_ARGS__);\
    			dbgprint(dbgbuff);

#endif //COMMON_H
