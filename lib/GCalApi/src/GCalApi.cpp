#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <algorithm>
#include "GCalApi.h"

#if defined(ESP32) || defined(ESP8266)
  #include <WiFiClientSecure.h>
#endif

static time_t parseIso8601ToEpoch(const String& s) {
  // Supports: "YYYY-MM-DD", "YYYY-MM-DDTHH:MM:SSZ", "YYYY-MM-DDTHH:MM:SS+07:00"
  if (s.length() < 10) return 0;
  int year = s.substring(0,4).toInt();
  int mon  = s.substring(5,7).toInt();
  int day  = s.substring(8,10).toInt();
  int hh=0, mm=0, ss=0;
  int tzSign = 0, tzH=0, tzM=0;

  if (s.length() >= 19 && s.charAt(10) == 'T') {
    hh = s.substring(11,13).toInt();
    mm = s.substring(14,16).toInt();
    ss = s.substring(17,19).toInt();
    if (s.endsWith("Z")) {
      tzSign = 0;
    } else {
      int p = s.indexOf('+', 19);
      if (p < 0) p = s.indexOf('-', 19);
      if (p > 0) {
        tzSign = (s.charAt(p) == '+') ? +1 : -1;
        tzH = s.substring(p+1, p+3).toInt();
        tzM = s.substring(p+4, p+6).toInt();
      }
    }
  } else {
    // all-day date only
  }

  struct tm t = {0};
  t.tm_year = year - 1900;
  t.tm_mon  = mon - 1;
  t.tm_mday = day;
  t.tm_hour = hh;
  t.tm_min  = mm;
  t.tm_sec  = ss;

  time_t epoch;
  #if defined(ESP32) || defined(ESP8266)
    epoch = timegm(&t); // UTC
  #else
    epoch = mktime(&t);
  #endif

  // If source had explicit offset like +07:00, it means local time = UTC + 7h -> epoch should subtract 7h to get UTC.
  long tzOffset = (tzSign == 0) ? 0 : (tzSign * (tzH*3600 + tzM*60));
  epoch -= tzOffset;
  return epoch;
}

static ApiError httpGetMaybeTLS(const String& url, String& body) {
  ApiError err;
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);

#if defined(ESP32) || defined(ESP8266)
  if (url.startsWith("https://")) {
    WiFiClientSecure client;
  #if GCAL_TLS_INSECURE
    client.setInsecure();
  #endif
    if (!http.begin(client, url)) {
      err.ok = false; err.httpCode = -1; err.message = F("http.begin TLS failed"); return err;
    }
  } else
#endif
  {
    if (!http.begin(url)) {
      err.ok = false; err.httpCode = -1; err.message = F("http.begin failed"); return err;
    }
  }

  int code = http.GET();
  if (code <= 0) { err.ok = false; err.httpCode = code; err.message = F("GET failed"); http.end(); return err; }
  body = http.getString();
  http.end();
  err.ok = (code == HTTP_CODE_OK);
  err.httpCode = code;
  if (!err.ok) err.message = F("Non-200 status");
  return err;
}

ApiError GCalApi::getUpcoming(std::vector<CalendarEvent>& out, uint8_t maxItems) {
  String url = GCAL_JSON_URL;
  if (url.indexOf('?') < 0) {
    url += "?max=" + String(maxItems);
  }

  String body;
  ApiError httpErr = httpGetMaybeTLS(url, body);
  if (!httpErr.ok) return httpErr;

  // Expecting: array of objects
  DynamicJsonDocument doc(12288); // 12 KB
  auto jerr = deserializeJson(doc, body);
  if (jerr) {
    ApiError e; e.ok = false; e.httpCode = 200; e.message = String("JSON error: ") + jerr.c_str(); return e;
  }

  if (!doc.is<JsonArray>()) {
    ApiError e; e.ok = false; e.httpCode = 200; e.message = F("JSON not array"); return e;
  }

  out.clear();
  for (JsonObject ev : doc.as<JsonArray>()) {
    CalendarEvent e;
    e.id       = (const char*)(ev["id"] | "");
    e.title    = (const char*)(ev["title"] | "");
    e.location = (const char*)(ev["location"] | "");

    // start / end may be epoch (number) or ISO-8601 (string)
    if (ev["start"].is<long long>() || ev["start"].is<double>()) {
      e.startUtc = (time_t) ev["start"].as<long long>();
    } else if (ev["start"].is<const char*>()) {
      e.startUtc = parseIso8601ToEpoch(ev["start"].as<const char*>());
    }

    if (ev["end"].is<long long>() || ev["end"].is<double>()) {
      e.endUtc = (time_t) ev["end"].as<long long>();
    } else if (ev["end"].is<const char*>()) {
      e.endUtc = parseIso8601ToEpoch(ev["end"].as<const char*>());
    }

    e.allDay   = ev["allDay"] | false;
    out.push_back(e);
    if (out.size() >= maxItems) break;
  }

  std::sort(out.begin(), out.end(), [](const CalendarEvent& a, const CalendarEvent& b){
    return a.startUtc < b.startUtc;
  });

  ApiError ok; ok.ok = true; ok.httpCode = 200; return ok;
}
