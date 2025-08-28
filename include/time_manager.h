#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>

void time_init();                 // config NTP
String time_now_string();         // "YYYY-MM-DD HH:MM:SS"
time_t time_now_epoch();          // epoch seconds

#endif
