#pragma once
#include "api_types.h"
#include "api_config.h"
#include "secrets.h"

namespace WeatherApi {
  ApiError getCurrent(WeatherData& out);
  const char* mapIconToAsset(const String& iconCode);
}
