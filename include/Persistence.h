#pragma once

#include <ArduinoJson.h>

/**
 * Write JSON without truncating the current known-good file first.
 *
 * The temporary file is fully written and size-checked before a same-filesystem
 * rename replaces the target. Both supported LittleFS implementations map that
 * rename to their atomic replacement operation.
 */
bool persistJsonAtomically(const char *targetPath, const char *temporaryPath,
                           JsonDocument &document);

/** Apply a staged two-file recovery transaction before configuration is read. */
bool applyPendingRestore();
