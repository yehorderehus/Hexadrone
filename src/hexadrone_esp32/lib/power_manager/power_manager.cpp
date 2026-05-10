#include "power_manager.h"
#include "radio_manager.h"

// Access the global radio object defined in main.cpp
extern RadioManager radio;

void PowerManager::begin()
{
    if (_ina.begin())
    {
        _ina.setMaxCurrentShunt(60, 0.0005); // Configured for 60A/0.5mOhm
        _ina.setAverage(INA228_16_SAMPLES);  // Options: 1, 4, 16, 64, 128, 256, 512, 1024
        _ina.setAccumulation(1);             // Reset mAh
        _sensorPresent = true;
        Blackbox.logSystem("[POWER] INA228 Initialized.");
    }
    else
    {
        _sensorPresent = false;
        Blackbox.logSystem("[POWER] WARNING: INA228 Missing. Using dummy battery data.");
    }
}

BatteryState PowerManager::update()
{
    static unsigned long _lastTelemetryTime = 0;
    unsigned long now = millis();

    if (now - _lastTelemetryTime >= TELEMETRY_INTERVAL)
    {
        _lastTelemetryTime = now;

        // Fetch fresh data from I2C bus only when needed
        PowerStats stats = read();

        // 1. Logging Logic (Delta check)
        if (abs(stats.voltage - _lastLoggedV) >= LOG_VOLTAGE_THRESH ||
            abs(stats.current - _lastLoggedI) >= LOG_CURRENT_THRESH)
        {
            Blackbox.logPower(stats.avgCell, stats.voltage, stats.current, stats.power, stats.mah);
            _lastLoggedV = stats.voltage;
            _lastLoggedI = stats.current;
        }

        // 2. Battery Percentage Math
        float clampedV = stats.voltage;
        if (clampedV > FULL_VOLTAGE) clampedV = FULL_VOLTAGE;
        if (clampedV < SOFT_CUTOFF_VOLTAGE) clampedV = SOFT_CUTOFF_VOLTAGE;

        uint8_t remaining_percent = (uint8_t)(((clampedV - SOFT_CUTOFF_VOLTAGE) /
                                               (FULL_VOLTAGE - SOFT_CUTOFF_VOLTAGE)) *
                                              100.0f);

        // 3. Send to Radio
        radio.sendBatteryTelemetry(stats.voltage, stats.current, stats.mah, remaining_percent);

        // 4. Update the persistent health state
        _currentHealthState = evaluateHealth(stats);
    }

    // Return the actual calculated state instead
    return _currentHealthState;
}

PowerStats PowerManager::read()
{
    PowerStats stats;
    if (!_sensorPresent)
    {
        stats.voltage = 0.0f;
        stats.current = 0.0f;
        stats.power = 0.0f;
        stats.mah = 0.0f;
        stats.avgCell = 0.0f;
        return stats;
    }

    stats.voltage = _ina.getBusVoltage();
    stats.current = _ina.getAmpere();
    stats.power = _ina.getPower();
    stats.mah = _ina.getCharge() / 3.6;
    stats.avgCell = (stats.voltage > 1.0f) ? (stats.voltage / 6.0f) : 0.0f;

    return stats;
}

/* void PowerManager::logPowerStats(PowerStats stats, int8_t rssi, int lq)
{
    Blackbox.printf(
        "%s\n%.1fV | %.1fV/c\n%.1fA | %.1fW\n%.0f mAh\n%ddBm | %d:100\n-----------------------\n",
        Blackbox.getTimestamp().c_str(),
        stats.voltage, stats.avgCell,
        stats.current, stats.power,
        stats.mah,
        (int)rssi, lq);
} */
// TODO: use for the display output, implement the function there or somewhere else

BatteryState PowerManager::evaluateHealth(const PowerStats &stats)
{
    // 1. USB/No-Battery Gate
    if (stats.voltage < IGNORE_VOLTAGE)
    {
        _isHard = false;
        _hardLogged = false;
        _isSoft = false;
        _softLogged = false;
        return BatteryState::NORMAL;
    }

    // 2. Hard Cutoff
    if (stats.voltage < HARD_CUTOFF_VOLTAGE)
    {
        if (!_isHard)
        {
            _isHard = true;
            _hardStartTime = millis();
        }

        if (millis() - _hardStartTime >= HARD_CUTOFF_INTERVAL)
        {
            if (!_hardLogged)
            {
                Blackbox.logSystem("[POWER] Hard Cutoff! Pulling power instantly.");
                _hardLogged = true;
            }
            return BatteryState::HARD_CUTOFF;
        }
    }
    else
    {
        _isHard = false;
        _hardLogged = false;
    }

    // 3. Soft Cutoff
    if (stats.voltage < SOFT_CUTOFF_VOLTAGE)
    {
        if (!_isSoft)
        {
            _isSoft = true;
            _softStartTime = millis();
        }

        if (millis() - _softStartTime >= SOFT_CUTOFF_INTERVAL)
        {
            if (!_softLogged)
            {
                Blackbox.logSystem("[POWER] Soft Cutoff! Forcing Disarm sequence.");
                _softLogged = true;
            }
            return BatteryState::SOFT_CUTOFF;
        }
    }
    else
    {
        _isSoft = false;
        _softLogged = false;
    }

    // 4. Voltage Warning
    if (stats.voltage < WARNING_VOLTAGE)
    {
        if (!_isVoltageWarning)
        {
            _isVoltageWarning = true;
            _voltageWarningStartTime = millis();
        }
        if (millis() - _voltageWarningStartTime >= VOLTAGE_WARNING_INTERVAL)
        {
            if (!_voltageWarningLogged)
            {
                Blackbox.logSystem("[POWER] Low Battery! Sagging below %.1fV (%.1fV)", WARNING_VOLTAGE, stats.voltage);
                _voltageWarningLogged = true;
            }
        }
    }
    else
    {
        _isVoltageWarning = false;
        _voltageWarningLogged = false;
    }

    // 5. Wattage Warning
    if (stats.power > WARNING_WATTAGE)
    {
        if (!_isWattageWarning)
        {
            _isWattageWarning = true;
            _wattageWarningStartTime = millis();
        }
        if (millis() - _wattageWarningStartTime >= WATTAGE_WARNING_INTERVAL)
        {
            if (!_wattageWarningLogged)
            {
                Blackbox.logSystem("[POWER] High Power! Consuming %.1fW (limit %.1fW)", stats.power, WARNING_WATTAGE);
                _wattageWarningLogged = true;
            }
        }
    }
    else
    {
        _isWattageWarning = false;
        _wattageWarningLogged = false;
    }

    return BatteryState::NORMAL;
}
