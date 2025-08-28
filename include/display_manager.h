#pragma once
#include <Arduino.h>

void display_init(bool isSH1106);
void display_show_status(const String& l1, const String& l2, const String& l3);
void display_show_subs(long subs);
void display_show_crypto(float btc, float eth);
void display_show_news(String news[3]);
