#pragma once
#include <Adafruit_PWMServoDriver.h>
#include <vector>
#include <config.h>
#include <logger.h>
#include <hexadrone_core/state_machine.hpp>

class ServoManager
{
public:
    // --- Core Interface ---
    void begin();
    void update(Hexadrone::DroneState currentState);
    void applyAngles(const std::vector<float> &angles_deg);
    void rapidKill();
    void setPostureState(Hexadrone::PostureState posture);
    bool isDisarming() const { return _isDisarming; }

private:
    // --- Hardware ---
    Adafruit_PWMServoDriver _board1 = Adafruit_PWMServoDriver(ADDR_SERVO_1);
    Adafruit_PWMServoDriver _board2 = Adafruit_PWMServoDriver(ADDR_SERVO_2);
    bool _pcaPresent = false;
    bool _killed = false;

    // --- State Trackers ---
    Hexadrone::DroneState _lastDroneState = Hexadrone::DroneState::DRONE_DISARMED;
    Hexadrone::PostureState _currentPosture = Hexadrone::PostureState::POSTURE_STANDARD;

    // --- Sequential Arming/Disarming ---
    int _currentActive = 0;
    int _targetActive = 0;
    unsigned long _lastArmTime = 0;
    unsigned long _disarmTriggerTime = 0;
    bool _isDisarming = false;

    // --- Helpers ---
    void setRawPWM(int index, int pulse);
    int degreesToTicks(float degrees);
};
