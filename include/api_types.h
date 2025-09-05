#pragma once
#include <Arduino.h>
#include <vector>
#include <time.h>

struct WeatherData {
  float   temp       = NAN;     // °C/°F (config)
  float   humidity   = NAN;     // %
  String  icon;                 // OWM: 01d, 02n, ...
  String  description;          // "mây rải rác", ...
  uint32_t fetchedAtMs = 0;     // millis() lúc fetch
};

struct CalendarEvent {
  String id;
  String title;
  time_t startUtc = 0;          // epoch giây UTC
  time_t endUtc   = 0;          // epoch giây UTC
  String location;
  bool   allDay   = false;
};

struct ApiError {
  bool   ok = true;
  int    httpCode = 0;
  String message;
};
