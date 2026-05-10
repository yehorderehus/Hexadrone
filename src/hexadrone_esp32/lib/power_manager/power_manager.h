#pragma once
#include <INA228.h>
#include <config.h>
#include <logger.h>

enum class BatteryState
{
    NORMAL,
    SOFT_CUTOFF,
    HARD_CUTOFF
};

struct PowerStats
{
    float voltage;
    float current;
    float power;
    float mah;
    float avgCell;
};

class PowerManager
{
public:
    void begin();
    BatteryState update();

private:
    INA228 _ina = INA228(ADDR_POWER);
    bool _sensorPresent = false;

    // Battery Cutoff Trackers
    unsigned long _hardStartTime = 0;
    bool _isHard = false;
    bool _hardLogged = false;
    unsigned long _softStartTime = 0;
    bool _isSoft = false;
    bool _softLogged = false;
    unsigned long _voltageWarningStartTime = 0;
    bool _isVoltageWarning = false;
    bool _voltageWarningLogged = false;
    unsigned long _wattageWarningStartTime = 0;
    bool _isWattageWarning = false;
    bool _wattageWarningLogged = false;

    // State trackers for delta logging
    float _lastLoggedV = 0.0f;
    float _lastLoggedI = 0.0f;
    float _lastLoggedMah = 0.0f;

    PowerStats read();
    // void logPowerStats(PowerStats stats); TODO: use for the display output, implement the function there or somewhere else
    BatteryState evaluateHealth(const PowerStats &stats);
    BatteryState _currentHealthState = BatteryState::NORMAL;
};
