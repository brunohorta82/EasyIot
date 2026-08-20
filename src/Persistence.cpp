#include "Persistence.h"

#include <LittleFS.h>

bool persistJsonAtomically(const char *targetPath, const char *temporaryPath,
                           JsonDocument &document)
{
  // ArduinoJson can keep a syntactically valid but incomplete document after
  // an allocation failure. Size checks would agree with that partial document,
  // so reject it before touching even the non-authoritative temporary file.
  if (document.overflowed())
    return false;

  // A stale temporary file is never authoritative and may be left behind by a
  // prior loss of power. Removing it cannot affect the known-good target.
  LittleFS.remove(temporaryPath);

  File temporary = LittleFS.open(temporaryPath, "w");
  if (!temporary)
    return false;

  const size_t expectedBytes = measureJson(document);
  const size_t writtenBytes = serializeJson(document, temporary);
  temporary.flush();
  temporary.close();

  if (writtenBytes != expectedBytes)
  {
    LittleFS.remove(temporaryPath);
    return false;
  }

  File verification = LittleFS.open(temporaryPath, "r");
  if (!verification)
  {
    LittleFS.remove(temporaryPath);
    return false;
  }
  const size_t storedBytes = verification.size();
  verification.close();
  if (storedBytes != expectedBytes)
  {
    LittleFS.remove(temporaryPath);
    return false;
  }

  // Do not remove the target first. A direct rename keeps the previous file
  // intact if replacement fails and is atomic within LittleFS.
  if (!LittleFS.rename(temporaryPath, targetPath))
  {
    LittleFS.remove(temporaryPath);
    return false;
  }
  return true;
}
