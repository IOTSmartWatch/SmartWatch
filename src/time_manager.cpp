#include "time_manager.h"
#include <WiFi.h>
#include <time.h>

void time_init() {
  if (WiFi.status() == WL_CONNECTED) {
    configTime(0, 0, "pool.ntp.org", "time.google.com");
  }
}
