#include "Shutters.h"
#include "Actuatores.h"
#define LEVEL_NONE 255
using namespace ShuttersInternal;

Shutters::Shutters(unsigned int downPin, unsigned int upPin, int actuatorId)
    : _upCourseTime(0), _downCourseTime(0), _calibrationRatio(0.1), _state(STATE_IDLE), _stateTime(0), _direction(DIRECTION_UP), _currentLevel(LEVEL_NONE), _targetLevel(LEVEL_NONE), _safetyDelay(false), _safetyDelayTime(0), _reset(true), _operationHandler(nullptr), _levelReachedCallback(nullptr)
{

  this->downPin = downPin;
  this->upPin = upPin;
  this->actuatorId = actuatorId;
}

void Shutters::_up()
{
  DPRINTLN(F("Shutters: going up"));
  _operationHandler(this, ShuttersOperation::UP);
}

void Shutters::_down()
{
  DPRINTLN(F("Shutters: going down"));
  _operationHandler(this, ShuttersOperation::DOWN);
}

void Shutters::_halt()
{
  DPRINTLN(F("Shutters: halting"));
  _operationHandler(this, ShuttersOperation::HALT);
  _setSafetyDelay();
}

void Shutters::_setSafetyDelay()
{
  _safetyDelayTime = millis();
  _safetyDelay = true;
}

void Shutters::_notifyLevel()
{
  DPRINT(F("Shutters: notifying level "));
  DPRINTLN(_currentLevel);
  if (_levelReachedCallback)
    _levelReachedCallback(this, _currentLevel);
}

uint32_t Shutters::getUpCourseTime()
{
  return _upCourseTime;
}

uint32_t Shutters::getDownCourseTime()
{
  return _downCourseTime;
}

Shutters &Shutters::setOperationHandler(ShuttersInternal::OperationHandler handler)
{
  _operationHandler = handler;

  return *this;
}

Shutters &Shutters::restoreState(uint8_t state, uint32_t downCourseTime, uint32_t upCourseTime)
{
  if (!_reset)
  {
    return *this;
  }
  _currentLevel = state;
  _downCourseTime = downCourseTime;
  _upCourseTime = upCourseTime;
  _notifyLevel();

  return *this;
}

Shutters &Shutters::setCourseTime(uint32_t upCourseTime, uint32_t downCourseTime)
{
  if (!_reset)
  {
    return *this;
  }

  if (upCourseTime > 67108864UL || upCourseTime == 0)
    return *this; // max value for 26 bits
  // if down course time is not set, consider it's the same as up
  if (downCourseTime == 0)
    downCourseTime = upCourseTime;
  if (downCourseTime > 67108864UL)
    return *this; // max value for 26 bits

  if (upCourseTime != _upCourseTime || downCourseTime != _downCourseTime)
  {
    DPRINTLN(F("Shutters: course time is not the same, invalidating stored state"));
    _currentLevel = LEVEL_NONE;
  }

  _upCourseTime = upCourseTime;
  _upStepTime = upCourseTime / LEVELS;
  _upCalibrationTime = upCourseTime * _calibrationRatio;

  _downCourseTime = downCourseTime;
  _downStepTime = downCourseTime / LEVELS;
  _downCalibrationTime = downCourseTime * _calibrationRatio;
  return *this;
}

float Shutters::getCalibrationRatio()
{
  return _calibrationRatio;
}

Shutters &Shutters::setCalibrationRatio(float calibrationRatio)
{
  _calibrationRatio = calibrationRatio;
  _upCalibrationTime = _upCourseTime * calibrationRatio;
  _downCalibrationTime = _downCourseTime * calibrationRatio;

  return *this;
}

Shutters &Shutters::onLevelReached(ShuttersInternal::LevelReachedCallback callback)
{
  _levelReachedCallback = callback;

  return *this;
}

Shutters &Shutters::begin()
{
  if (_upCourseTime == 0 || _downCourseTime == 0)
    return *this;

  _reset = false;
  return *this;
}

Shutters &Shutters::setLevel(uint8_t level)
{
  if (_reset)
  {
    return *this;
  }

  if (level > 100)
  {
    return *this;
  }

  if (_state == STATE_IDLE && level == _currentLevel)
    return *this;
  if ((_state == STATE_TARGETING || _state == STATE_NORMALIZING) && level == _targetLevel)
    return *this; // normalizing check useless, but avoid following lines overhead

  _targetLevel = level;
  Direction direction = (_targetLevel > _currentLevel) ? DIRECTION_DOWN : DIRECTION_UP;
  if (_state == STATE_TARGETING && _direction != direction)
  {
    _state = STATE_NORMALIZING;
  }

  return *this;
}

Shutters &Shutters::stop()
{
  if (_reset)
    return *this;

  if (_state == STATE_IDLE)
    return *this;

  _targetLevel = LEVEL_NONE;
  if (_state == STATE_TARGETING)
  {
    _state = STATE_NORMALIZING;
  }

  return *this;
}

Shutters &Shutters::loop()
{
  if (_reset)
    return *this;

  if (_safetyDelay)
  {
    if (millis() - _safetyDelayTime >= SAFETY_DELAY)
    {
      DPRINTLN(F("Shutters: end of safety delay"));
      _safetyDelay = false;
    }

    return *this;
  }

  // here, we're safe for relays

  if (_currentLevel == LEVEL_NONE)
  {
    if (_state != STATE_RESETTING)
    {
      DPRINTLN(F("Shutters: level not known, resetting"));
      _up();
      _state = STATE_RESETTING;
      _stateTime = millis();
    }
    else if (millis() - _stateTime >= _upCourseTime + _upCalibrationTime)
    {
      DPRINTLN(F("Shutters: level now known"));
      _halt();
      _state = STATE_IDLE;
      _currentLevel = 0;
      _notifyLevel();
    }

    return *this;
  }

  // here, level is known

  if (_state == STATE_IDLE && _targetLevel == LEVEL_NONE)
    return *this; // nothing to do

  if (_state == STATE_CALIBRATING)
  {
    const uint32_t calibrationTime = (_direction == DIRECTION_UP) ? _upCalibrationTime : _downCalibrationTime;
    if (millis() - _stateTime >= calibrationTime)
    {
      DPRINTLN(F("Shutters: calibration is done"));
      _halt();
      _state = STATE_IDLE;
      _notifyLevel();
    }

    return *this;
  }

  // here, level is known and calibrated, and we need to do something

  if (_state == STATE_IDLE)
  {
    DPRINTLN(F("Shutters: starting move"));
    _direction = (_targetLevel > _currentLevel) ? DIRECTION_DOWN : DIRECTION_UP;
    (_direction == DIRECTION_UP) ? _up() : _down();
    _state = STATE_TARGETING;
    _stateTime = millis();

    return *this;
  }

  // here, we have to handle targeting and normalizing

  const uint32_t stepTime = (_direction == DIRECTION_UP) ? _upStepTime : _downStepTime;

  if (millis() - _stateTime < stepTime)
    return *this;

  _currentLevel += (_direction == DIRECTION_UP) ? -1 : 1;
  _stateTime = millis();

  if (_currentLevel == 0 || _currentLevel == 100)
  { // we need to calibrate
    DPRINTLN(F("Shutters: starting calibration"));
    _state = STATE_CALIBRATING;
    if (_currentLevel == _targetLevel)
      _targetLevel = LEVEL_NONE;

    return *this;
  }

  if (_state == STATE_NORMALIZING)
  { // we've finished normalizing
    DPRINTLN(F("Shutters: finished normalizing"));
    _halt();
    _state = STATE_IDLE;
    _notifyLevel();

    return *this;
  }

  if (_state == STATE_TARGETING && _currentLevel == _targetLevel)
  { // we've reached out target
    DPRINTLN(F("Shutters: reached target"));
    _halt();
    _state = STATE_IDLE;
    _targetLevel = LEVEL_NONE;
    _notifyLevel();

    return *this;
  }

  // we've reached an intermediary level

  _notifyLevel();

  return *this;
}

bool Shutters::isIdle()
{
  return _state == STATE_IDLE;
}
bool Shutters::isCalibration()
{
  return _state == STATE_CALIBRATING;
}
uint8_t Shutters::getCurrentLevel()
{
  return _currentLevel;
}

Shutters &Shutters::reset()
{
  _halt();
  _reset = true;

  return *this;
}

bool Shutters::isReset()
{
  return _reset;
}
