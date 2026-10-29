#ifndef AQUADANCE_H
#define AQUADANCE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

/**
 * AquaDance / Fontaine musical water choreography sequencer.
 *
 * Transforms garden / irrigation valves into synchronized musical fountain jets.
 * Each show consists of tracks (one per valve) and time steps forming a sequence matrix.
 */

constexpr size_t kMaxAquaShows{8};
constexpr uint16_t kMaxAquaSteps{64};
constexpr uint16_t kMinStepMs{50};
constexpr uint16_t kMaxStepMs{5000};
constexpr uint16_t kDefaultStepMs{400};

enum AquaTrackType : uint8_t
{
  TRACK_VALVE = 0,
  TRACK_LIGHT_DIMMER = 1,
  TRACK_LIGHT_RGBW = 2
};

struct AquaTrack
{
  char uniqueId[24]{};
  uint8_t trackType{TRACK_VALVE};
  float posX{50.0f}; // 0.0 to 100.0 percent
  float posY{50.0f}; // 0.0 to 100.0 percent
  std::vector<uint8_t> steps;  // For valve: 0/1. For dimmer: 0-100% power.
  std::vector<uint32_t> rgbw;  // For RGBW color (0x00RRGGBB or 0xWWRRGGBB)
};

struct AquaShow
{
  uint8_t id{1};
  char name[32]{"AquaDance"};
  uint16_t stepMs{kDefaultStepMs};
  uint16_t totalSteps{32};
  bool loop{false};
  std::vector<AquaTrack> tracks;
};

class AquaDance
{
public:
  bool enabled{true};
  std::vector<AquaShow> shows;

  /** Reads /aquadance.json. */
  void load();
  bool save();

  /** Updates the whole in-memory shows list from JSON. */
  bool update(JsonObject &root);

  /** Serialises state under 'aquadance' key for /config. */
  void json(JsonVariant &root);

  /** Serialises state and shows for API responses. */
  void jsonBody(JsonVariant &root);

  /** Real-time status including current playback state and step index. */
  void statusJson(JsonVariant &root);

  /** Starts playing a show by id. */
  bool play(uint8_t showId);

  /** Stops playback and closes all fountain valves. */
  void stop();

  bool isRunning() const { return runningShow >= 0; }
  uint8_t runningShowId() const;
  uint16_t currentStepIndex() const { return currentStep; }

  /** Step advancing loop, called from main/actuator loop. */
  void loop();

  /** Applies a command payload e.g. "RUN:<id>", "STOP". */
  bool command(const char *payload);

private:
  int runningShow{-1};        // index in shows vector
  uint16_t currentStep{0};
  unsigned long nextStepAt{0};

  void applyCurrentStep();
  void closeAllShowValves();
  void clearRuntime();
  const AquaShow *running() const;
};

extern AquaDance aquadance;

#endif
