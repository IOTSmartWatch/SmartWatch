#pragma once

// ---- OpenWeatherMap config ----
#define OWM_LAT    "10.7769"
#define OWM_LON    "106.7009"
#define OWM_UNITS  "metric"     // or "imperial"
#define OWM_LANG   "vi"

// ---- Intervals ----
#define WEATHER_REFRESH_MS  (15UL * 60UL * 1000UL)
#define GCAL_REFRESH_MS     (10UL * 60UL * 1000UL)

// ---- Network ----
#define HTTP_TIMEOUT_MS     8000
#define GCAL_TLS_INSECURE   1     // 1: skip certificate check (dev/test), 0: use proper CA

// ---- Timezone (for formatting when needed) ----
#define IANA_TZ             "Asia/Ho_Chi_Minh"
