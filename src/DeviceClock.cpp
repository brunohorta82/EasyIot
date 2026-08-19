#include "DeviceClock.h"
#include "Constants.h"
#include <time.h>

namespace
{
// Anything before this is the epoch default, meaning NTP has not answered yet.
// 2024-01-01 is comfortably in the past and comfortably after 1970.
constexpr time_t kPlausibleAfter = 1704067200;

bool readLocalTime(struct tm &out)
{
  time_t now = time(nullptr);
  if (now < kPlausibleAfter)
  {
    return false;
  }
  localtime_r(&now, &out);
  return true;
}
} // namespace

void setupDeviceClock()
{
  // The build already carries both values; they were simply never applied.
  configTzTime(TZ_INFO, NTP_SERVER);
}

bool clockSynced()
{
  struct tm t{};
  return readLocalTime(t);
}

int clockMinuteOfDay()
{
  struct tm t{};
  if (!readLocalTime(t))
  {
    return -1;
  }
  return t.tm_hour * 60 + t.tm_min;
}

int clockWeekday()
{
  struct tm t{};
  if (!readLocalTime(t))
  {
    return -1;
  }
  return t.tm_wday;
}

String clockNowIso()
{
  struct tm t{};
  if (!readLocalTime(t))
  {
    return String();
  }
  char buf[25];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &t);
  return String(buf);
}
