# Smartwatch – Weather & Google Calendar API (ESP32 + SH1106)

**Mục tiêu:** Module hoá **OpenWeatherMap (thời tiết)** và **Google Calendar (sự kiện)** → **parse JSON → struct**.

**Phạm vi:** Chỉ phần API + ví dụ gắn với UI SH1106 hiện có (không thiết kế UI mới).

---

## 1) Cấu trúc dự án

```
include/
  api_types.h      // struct: WeatherData, CalendarEvent, ApiError
  api_config.h     // LAT/LON, UNITS, LANG, intervals, timeout, TLS flag
  secrets.h        // KHÔNG commit (mẫu: secrets.h.example)
  secrets.h.example
lib/
  WeatherApi/
    src/WeatherApi.h
    src/WeatherApi.cpp
  GCalApi/
    src/GCalApi.h
    src/GCalApi.cpp
src/
  main.cpp         // ví dụ gọi API và render lên SH1106 (UI của team)
```

---

## 2) Cài nhanh (PlatformIO)

**platformio.ini** – thêm (nếu repo chưa có):

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

lib_deps =
  bblanchon/ArduinoJson @ ^7
  adafruit/Adafruit GFX Library
  adafruit/Adafruit SH110X
  PaulStoffregen/Time
```

**Secrets**

1. Copy `include/secrets.h.example` → `include/secrets.h`
2. Điền:

```cpp
#define WIFI_SSID   "..."
#define WIFI_PASS   "..."
#define OWM_API_KEY "..."
#define GCAL_JSON_URL "https://<apps-script-or-cloud-function>/events"
```

> Lưu ý: **Không commit `secrets.h`** (thêm vào `.gitignore`).

Build & Upload. **Short press** nút → màn tiếp; **Long press** → màn trước.

---

## 3) Cấu hình nhanh (`api_config.h`)

```cpp
#define OWM_LAT   "10.7769"
#define OWM_LON   "106.7009"
#define OWM_UNITS "metric"     // hoặc "imperial"
#define OWM_LANG  "vi"

#define WEATHER_REFRESH_MS  (15UL*60UL*1000UL)
#define GCAL_REFRESH_MS     (10UL*60UL*1000UL)

#define HTTP_TIMEOUT_MS     8000
#define GCAL_TLS_INSECURE   1        // dev/test: 1 (bỏ kiểm tra cert). Prod: 0.

#define IANA_TZ "Asia/Ho_Chi_Minh"
```

---

## 4) Hợp đồng dữ liệu (struct)

```cpp
struct WeatherData {
  float temp, humidity;    // °C/°F; %
  String icon, description;
  uint32_t fetchedAtMs;    // millis()
};

struct CalendarEvent {
  String id, title, location;
  time_t startUtc, endUtc; // epoch giây (UTC)
  bool   allDay;
};

struct ApiError { bool ok; int httpCode; String message; };
```

---

## 5) JSON contract (đầu vào từ server)

**OpenWeatherMap (current):**

- Endpoint: `http://api.openweathermap.org/data/2.5/weather?lat=...&lon=...&units=metric&lang=vi&appid=KEY`
- Trường dùng:

  - `main.temp` → `temp`
  - `main.humidity` → `humidity`
  - `weather[0].icon` → `icon` (vd: `01d`)
  - `weather[0].description` → `description`

**Google Calendar (qua JSON proxy – khuyến nghị):**

- `GCAL_JSON_URL` trả mảng:

```json
[
  {
    "id": "abc",
    "title": "Standup",
    "start": 1725603600,
    "end": 1725605400,
    "location": "Zoom",
    "allDay": false
  }
]
```

> Hỗ trợ **epoch** (khuyến nghị) hoặc **ISO‑8601** (`...Z`, `+07:00`) → code tự chuyển sang epoch UTC.

---

## 6) Cách dùng API (ví dụ)

```cpp
#include "WeatherApi.h"
#include "GCalApi.h"
#include "api_types.h"

WeatherData w;
std::vector<CalendarEvent> evs;

ApiError e1 = WeatherApi::getCurrent(w);
if (!e1.ok) Serial.printf("Weather err %d %s\n", e1.httpCode, e1.message.c_str());

ApiError e2 = GCalApi::getUpcoming(evs, 5);
if (!e2.ok) Serial.printf("GCal err %d %s\n", e2.httpCode, e2.message.c_str());

// Sau đó UI của bạn render: temp/humidity/desc và danh sách events
```

---

## 7) Test & lưu ý

- **Wokwi**: dùng `Wokwi-GUEST` (pass trống). OWM có thể gọi **HTTP** → test nhanh.
- **HTTPS Calendar**: dev để `GCAL_TLS_INSECURE 1`; **prod** nạp **root CA** rồi `setCACert(...)`.
- **Lỗi mạng/JSON**: trả `ApiError.ok=false` (không crash); giữ dữ liệu cũ để UI không trống.
- **RAM parse**: OWM ~4KB; GCal 5–10 events ~8–12KB.

---

## 8) Done khi

- Lấy thời tiết & sự kiện **thành công**, lỗi **được báo** (không reset).
- UI `SCR_WEATHER` & `SCR_EVENTS` dùng **dữ liệu thật** từ struct (không đổi kiến trúc vẽ).

#pragma once

// WiFi (dev/Wokwi)
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASS ""

// OpenWeatherMap
#define OWM_API_KEY "YOUR_OPENWEATHERMAP_KEY"

// Google Calendar JSON proxy (Apps Script / Cloud Function)
// Endpoint nên trả JSON mảng như trong README thiết kế.
#define GCAL_JSON_URL "https://your-apps-script-webapp.example.com/events"
