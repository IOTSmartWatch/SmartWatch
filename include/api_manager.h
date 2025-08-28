#pragma once
#include <Arduino.h>

struct ApiData {
  long  subs;
  float btc;
  float eth;
  String news[3]; // 3 English headlines
};

bool fetch_api(const char* url, ApiData &data);
