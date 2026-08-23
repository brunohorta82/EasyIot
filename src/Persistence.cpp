#include "Persistence.h"
#include "Constants.h"

#include <LittleFS.h>

namespace
{
bool persistJsonValueAtomically(const char *targetPath,
                                const char *temporaryPath,
                                JsonVariantConst value)
{
  // A stale temporary file is never authoritative and may be left behind by a
  // prior loss of power. Removing it cannot affect the known-good target.
  LittleFS.remove(temporaryPath);

  File temporary = LittleFS.open(temporaryPath, "w");
  if (!temporary)
    return false;

  const size_t expectedBytes = measureJson(value);
  const size_t writtenBytes = serializeJson(value, temporary);
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

} // namespace

bool persistJsonAtomically(const char *targetPath, const char *temporaryPath,
                           JsonDocument &document)
{
  // ArduinoJson can keep a syntactically valid but incomplete document after
  // an allocation failure. Size checks would agree with that partial document,
  // so reject it before touching even the non-authoritative temporary file.
  if (document.overflowed())
    return false;
  return persistJsonValueAtomically(targetPath, temporaryPath,
                                    document.as<JsonVariantConst>());
}

namespace
{
bool restoreRollbackFile(const char *target, const char *rollback)
{
  if (!LittleFS.exists(rollback))
    return true;
  LittleFS.remove(target);
  return LittleFS.rename(rollback, target);
}

bool beginRollbackFile(const char *target, const char *rollback, bool existed)
{
  LittleFS.remove(rollback);
  if (!existed)
  {
    LittleFS.remove(target);
    return true;
  }
  return LittleFS.exists(target) && LittleFS.rename(target, rollback);
}
} // namespace

bool applyPendingRestore()
{
  if (!LittleFS.exists(configFilenames::restore))
    return true;

  File file = LittleFS.open(configFilenames::restore, "r");
  JsonDocument transaction;
  const DeserializationError error = deserializeJson(transaction, file);
  file.close();
  if (error || strcmp(transaction["format"] | "", "easyiot-restore-transaction") != 0 ||
      (transaction["version"] | 0u) != 1u ||
      !transaction["config"].is<JsonObject>() ||
      !transaction["irrigation"].is<JsonObject>())
    return false;

  // Once both authoritative files were written, a committed journal means a
  // power cut merely interrupted cleanup. Never roll a completed recovery back.
  if (transaction["committed"] | false)
  {
    LittleFS.remove(configFilenames::configRollback);
    LittleFS.remove(configFilenames::irrigationRollback);
    LittleFS.remove(configFilenames::restore);
    return true;
  }

  const bool hadConfig = transaction["hadConfig"] | false;
  const bool hadIrrigation = transaction["hadIrrigation"] | false;

  // A surviving rollback file proves a previous boot was interrupted after the
  // transaction began. Reconstruct the exact baseline before retrying.
  if (!restoreRollbackFile(configFilenames::config,
                           configFilenames::configRollback) ||
      !restoreRollbackFile(configFilenames::irrigation,
                           configFilenames::irrigationRollback))
    return false;
  if (!hadConfig)
    LittleFS.remove(configFilenames::config);
  if (!hadIrrigation)
    LittleFS.remove(configFilenames::irrigation);

  if (!beginRollbackFile(configFilenames::config,
                         configFilenames::configRollback, hadConfig) ||
      !beginRollbackFile(configFilenames::irrigation,
                         configFilenames::irrigationRollback, hadIrrigation))
  {
    restoreRollbackFile(configFilenames::config,
                        configFilenames::configRollback);
    restoreRollbackFile(configFilenames::irrigation,
                        configFilenames::irrigationRollback);
    return false;
  }

  const bool configStored = persistJsonValueAtomically(
      configFilenames::config, configFilenames::configTemporary,
      transaction["config"].as<JsonVariantConst>());
  const bool irrigationStored = configStored && persistJsonValueAtomically(
      configFilenames::irrigation, configFilenames::irrigationTemporary,
      transaction["irrigation"].as<JsonVariantConst>());
  if (!configStored || !irrigationStored)
  {
    LittleFS.remove(configFilenames::config);
    LittleFS.remove(configFilenames::irrigation);
    restoreRollbackFile(configFilenames::config,
                        configFilenames::configRollback);
    restoreRollbackFile(configFilenames::irrigation,
                        configFilenames::irrigationRollback);
    return false;
  }

  // Persist the commit decision before deleting rollback data. Recovery after
  // any later power cut can only finish cleanup, never resurrect old files.
  transaction["committed"] = true;
  if (!persistJsonAtomically(configFilenames::restore,
                             configFilenames::restoreTemporary, transaction))
  {
    LittleFS.remove(configFilenames::config);
    LittleFS.remove(configFilenames::irrigation);
    restoreRollbackFile(configFilenames::config,
                        configFilenames::configRollback);
    restoreRollbackFile(configFilenames::irrigation,
                        configFilenames::irrigationRollback);
    return false;
  }

  LittleFS.remove(configFilenames::configRollback);
  LittleFS.remove(configFilenames::irrigationRollback);
  LittleFS.remove(configFilenames::restore);
  return true;
}
