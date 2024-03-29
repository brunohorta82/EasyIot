#pragma once

#include "Arduino.h"
#include "Actuatores.h"
#include "ShuttersOperation.hpp"
#include <vector>

// #define DEBUG_ONOFRE
#ifdef DEBUG_ONOFRE
#define DPRINT(...) Serial.print(__VA_ARGS__)
#define DPRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DPRINT(...)
#define DPRINTLN(...)
#endif

class Shutters;

namespace ShuttersInternal
{
  const uint16_t SAFETY_DELAY = 1 * 1000;
  const uint8_t LEVELS = 100;

  enum State : uint8_t
  {
    STATE_IDLE,        // not moving
    STATE_RESETTING,   // when state not known, goes to 0
    STATE_TARGETING,   // when going to target
    STATE_NORMALIZING, // when target changed, goes to next known level
    STATE_CALIBRATING  // when 0 or 100, to ensure actually at the end
  };
  enum Direction : bool
  {
    DIRECTION_DOWN,
    DIRECTION_UP
  };

  typedef void (*OperationHandler)(::Shutters *s, ::ShuttersOperation operation);
  typedef void (*LevelReachedCallback)(::Shutters *s, uint8_t level);
} // namespace ShuttersInternal

class Shutters
{
private:
  uint32_t _upCourseTime;
  uint32_t _downCourseTime;
  float _calibrationRatio;
  uint32_t _upStepTime;
  uint32_t _downStepTime;
  uint32_t _upCalibrationTime;
  uint32_t _downCalibrationTime;

  ShuttersInternal::State _state;
  uint32_t _stateTime;
  ShuttersInternal::Direction _direction;
  uint8_t _currentLevel;
  uint8_t _targetLevel;

  bool _safetyDelay;
  uint32_t _safetyDelayTime;

  bool _init;
  bool _reset;

  ShuttersInternal::OperationHandler _operationHandler;
  ShuttersInternal::LevelReachedCallback _levelReachedCallback;

  void _up();
  void _down();
  void _halt();
  void _setSafetyDelay();
  void _notifyLevel();

public:
  Shutters(unsigned int downPin, unsigned int upPin, int actuatorId);
  unsigned int downPin;
  unsigned int upPin;
  int actuatorId;
  uint32_t getUpCourseTime();
  uint32_t getDownCourseTime();
  Shutters &setOperationHandler(ShuttersInternal::OperationHandler handler);
  uint8_t getStateLength();
  Shutters &restoreState(uint8_t state, uint32_t downCourseTime, uint32_t upCourseTime);
  Shutters &setCourseTime(uint32_t upCourseTime, uint32_t downCourseTime = 0);
  float getCalibrationRatio();
  Shutters &setCalibrationRatio(float calibrationRatio);
  Shutters &onLevelReached(ShuttersInternal::LevelReachedCallback callback);
  Shutters &begin();
  Shutters &setLevel(uint8_t level);
  Shutters &stop();
  Shutters &loop();
  bool isIdle();
  bool isCalibration();
  uint8_t getCurrentLevel();
  Shutters &reset();
  bool isReset();
};
