#include "thingspeak_manager.h"
#include <HTTPClient.h>

static String url_encode(const String& s) {
  String out;
  const char* hex = "0123456789ABCDEF";
  for (size_t i=0; i<s.length(); ++i) {
    char c = s[i];
    if (('a'<=c && c<='z') || ('A'<=c && c<='Z') || ('0'<=c && c<='9') || c=='-'||c=='_'||c=='.'||c=='~') {
      out += c;
    } else {
      out += '%';
      out += hex[(c >> 4) & 0xF];
      out += hex[c & 0xF];
    }
  }
  return out;
}

bool ts_send_update(const char* writeKey, long subs, float btc, float eth, const String& rssTitle) {
  HTTPClient http;
  String url = String("http://api.thingspeak.com/update?api_key=") + writeKey +
               "&field1=" + String(subs) +
               "&field2=" + String(btc, 2) +
               "&field3=" + String(eth, 2) +
               "&field4=" + url_encode(rssTitle);
  http.begin(url);
  int code = http.GET();
  http.end();
  return code > 0;
}
