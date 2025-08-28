#pragma once
#include <Arduino.h>

bool ts_send_update(const char* writeKey, long subs, float btc, float eth, const String& rssTitle);
