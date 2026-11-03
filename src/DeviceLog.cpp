#include "DeviceLog.h"
#include <stdarg.h>
#include <stdlib.h>
#ifdef ESP32
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#endif

namespace
{
#ifdef ESP8266
  char *buffer{nullptr};
#else
  char buffer[kDeviceLogLines][kDeviceLogLineSize];
#endif
  size_t nextLine{0};
  bool wrapped{false};

#ifdef ESP32
  // The ring buffer is written from the main loop, from the sensor task, and --
  // since the cloud sync moved off the loop -- from the cloud task too. Two
  // writers racing on nextLine overwrite each other's line and can leave the
  // index past the end. The ESP8266 has one thread and needs none of this.
  SemaphoreHandle_t logMutex{nullptr};

  struct LogGuard
  {
    bool held{false};
    LogGuard()
    {
      if (logMutex == nullptr)
        logMutex = xSemaphoreCreateMutex();
      // A reader or writer that cannot take the lock within a tick writes
      // nothing rather than blocking something more important than a log line.
      held = logMutex != nullptr && xSemaphoreTake(logMutex, pdMS_TO_TICKS(20)) == pdTRUE;
    }
    ~LogGuard()
    {
      if (held)
        xSemaphoreGive(logMutex);
    }
  };
#else
  struct LogGuard
  {
    bool held{true};
  };
#endif

  bool ensureBuffer()
  {
#ifdef ESP8266
    if (buffer == nullptr)
    {
      buffer = static_cast<char *>(calloc(kDeviceLogLines, kDeviceLogLineSize));
      if (buffer == nullptr)
        return false;
      nextLine = 0;
      wrapped = false;
    }
#endif
    return true;
  }

  char *lineAt(size_t index)
  {
#ifdef ESP8266
    return buffer + index * kDeviceLogLineSize;
#else
    return buffer[index];
#endif
  }

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
  LogGuard guard;
  char stamp[12];
  stampInto(stamp, sizeof(stamp));

  char fallback[kDeviceLogLineSize] = {};
  const bool storeLine = ensureBuffer();
  char *line = storeLine ? lineAt(nextLine) : fallback;
  const int written = snprintf(line, kDeviceLogLineSize, "%s ", stamp);
  if (written > 0 && (size_t)written < kDeviceLogLineSize)
  {
    va_list args;
    va_start(args, format);
    vsnprintf(line + written, kDeviceLogLineSize - written, format, args);
    va_end(args);
  }

  if (storeLine)
  {
    nextLine = (nextLine + 1) % kDeviceLogLines;
    if (nextLine == 0)
      wrapped = true;
  }

#ifdef DEBUG_ONOFRE
  // Also on the wire when someone is actually looking at it.
  Serial.println(line);
#endif
}

String deviceLogText()
{
  LogGuard guard;
  String out;
  if (!ensureBuffer())
    return out;
  // Reserve once: growing a String line by line on an ESP8266 is how the heap
  // gets fragmented by the very code meant to help diagnose it.
  const size_t count = wrapped ? kDeviceLogLines : nextLine;
  out.reserve(count * 48 + 32);
  const size_t start = wrapped ? nextLine : 0;
  for (size_t i = 0; i < count; i++)
  {
    const char *line = lineAt((start + i) % kDeviceLogLines);
    if (line[0] == '\0')
      continue;
    out += line;
    out += '\n';
  }
  return out;
}

void deviceLogClear()
{
  LogGuard guard;
  if (!ensureBuffer())
    return;
  nextLine = 0;
  wrapped = false;
  for (size_t i = 0; i < kDeviceLogLines; i++)
    lineAt(i)[0] = '\0';
}

void deviceLogReleaseForUpdate()
{
#ifdef ESP8266
  free(buffer);
  buffer = nullptr;
  nextLine = 0;
  wrapped = false;
#endif
}
