#ifndef __TIME_UTILS_H
#define __TIME_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
extern time_t sntp_time;
extern int sntp_tz;

typedef struct {
	int year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
} DateTime;

char *epoch_to_str(unsigned long epoch);
DateTime GetDateTime(unsigned long epoch);
DateTime Now();

#ifdef __cplusplus
}
#endif

#endif
