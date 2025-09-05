#pragma once
#include "api_types.h"
#include "api_config.h"
#include "secrets.h"

namespace GCalApi {
  ApiError getUpcoming(std::vector<CalendarEvent>& out, uint8_t maxItems);
}
