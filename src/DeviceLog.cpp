#include "DeviceLog.h"
#include <stdarg.h>

namespace
{
  char buffer[kDeviceLogLines][kDeviceLogLineSize];
  size_t nextLine{0};
  bool wrapped{false};

  /** Milliseconds since boot as h:mm:ss, which is what makes a line placeable:
      "at 0:00:00" is the boot itself, "at 2:14:07" is something that happened
      while nobody was watching. */
  void stampInto(char *out, size_t size)
  {
    const unsigned long ms = millis();
    const unsigned long total = ms / 1000ul;
    snprintf(out, size, "%lu:%02lu:%02lu", total / 3600ul, (total / 60ul) % 60ul, total % 60ul);
  }
}

void deviceLog(const char *format, ...)
{
  char stamp[12];
  stampInto(stamp, sizeof(stamp));

  char *line = buffer[nextLine];
  const int written = snprintf(line, kDeviceLogLineSize, "%s ", stamp);
  if (written > 0 && (size_t)written < kDeviceLogLineSize)
  {
    va_list args;
    va_start(args, format);
    vsnprintf(line + written, kDeviceLogLineSize - written, format, args);
    va_end(args);
  }

  nextLine = (nextLine + 1) % kDeviceLogLines;
  if (nextLine == 0)
    wrapped = true;

#ifdef DEBUG_ONOFRE
  // Also on the wire when someone is actually looking at it.
  Serial.println(line);
#endif
}

String deviceLogText()
{
  String out;
  // Reserve once: growing a String line by line on an ESP8266 is how the heap
  // gets fragmented by the very code meant to help diagnose it.
  const size_t count = wrapped ? kDeviceLogLines : nextLine;
  out.reserve(count * 48 + 32);
  const size_t start = wrapped ? nextLine : 0;
  for (size_t i = 0; i < count; i++)
  {
    const char *line = buffer[(start + i) % kDeviceLogLines];
    if (line[0] == '\0')
      continue;
    out += line;
    out += '\n';
  }
  return out;
}

void deviceLogClear()
{
  nextLine = 0;
  wrapped = false;
  for (size_t i = 0; i < kDeviceLogLines; i++)
    buffer[i][0] = '\0';
}
