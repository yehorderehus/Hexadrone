#include <Arduino.h>
#include <Wire.h>
#include <config.h>
#include <logger.h>
#include <comms_manager.h>
#include <power_manager.h>
#include <radio_manager.h>
#include <servo_manager.h>
#include <hexadrone_core/brain.hpp>

CommsManager comms;
PowerManager power;
RadioManager radio;
ServoManager servo;
Hexadrone::Brain brain;

// --- Timing ---

static unsigned long _lastLoopTime = 0;

static float getDt()
{
    unsigned long now = millis();
    unsigned long delta = now - _lastLoopTime;

    if (delta == 0)
        return 0.001f; // Force 1ms minimum

    float dt = delta / 1000.0f;
    _lastLoopTime = now;
    return dt;
}

// --- Setup / Loop ---

void setup()
{
    Serial.begin(SERIAL_BAUD);
    Wire.begin(SDA_CUSTOM, SCL_CUSTOM);
    Wire.setClock(400000); // 400kHz I2C speed

    // Set to 10ms so it doesn't freeze the loop:
    Serial.setTimeout(10);
    Wire.setTimeOut(10);

    Blackbox.begin();

    power.begin(); // INA228 has to be initialized first
    comms.begin(WIFI_SSID, WIFI_PASS);
    radio.begin();
    servo.begin();

    _lastLoopTime = millis();

    Blackbox.logSystem("[SYSTEM] Hexadrone Initialized.");
}

void loop()
{
    // --- 1. OTA Mode Check ---
    static Hexadrone::DroneState lastNormalState = Hexadrone::DroneState::DRONE_DISARMED;
    static bool radio_link_active = false;
    static bool arm_switch_released_since_link = false;
    float dt = getDt();
    //    if (dt > 0.025f)
    //        Blackbox.logSystem("[SYSTEM] Loop Time Spike detected: %.3f sec", dt);

    bool otaLockdown = comms.isOTALockdown();
    if (otaLockdown)
    {
        // OTA mode: freeze legs, ignore radio, skip I2C sensors, keep WiFi alive
        Hexadrone::DroneState current_state = lastNormalState;
        comms.update(current_state);
        // Skip all other subsystems while OTA upload is in progress.
        return;
    }

    // --- 2. Data Acquisition ---
    radio.update();
    Hexadrone::ControllerInput input = radio.buildInput();

    // --- 3. Safety/Interlocks ---
    if (!radio.isConnected())
    {
        // If no radio link is present, always disarm the drone
        input.armed_switch = -1;
        radio_link_active = false;
        arm_switch_released_since_link = false;
    }
    else
    {
        // On first connect, require the ARM switch to be moved to DISARM before allowing ARMED state
        if (!radio_link_active)
        {
            radio_link_active = true;
            arm_switch_released_since_link = false;
        }
        if (input.armed_switch == -1)
        {
            arm_switch_released_since_link = true;
        }
        if (!arm_switch_released_since_link)
        {
            input.armed_switch = -1;
        }
    }

    BatteryState batt_state = power.update();
    if (batt_state == BatteryState::SOFT_CUTOFF || batt_state == BatteryState::HARD_CUTOFF)
    {
        // Force the Brain to think the physical switch is disarmed
        input.armed_switch = -1;
    }

    // --- 4. State Update ---
    std::vector<float> angles = brain.update(dt, input);
    Hexadrone::DroneState current_state = brain.getState();
    lastNormalState = current_state;

    // --- 5. Subsystem Updates ---
    comms.update(current_state);
    if (current_state == Hexadrone::DroneState::DRONE_OE_KILLED || batt_state == BatteryState::HARD_CUTOFF)
    {
        servo.rapidKill();
    }
    else
    {
        // Update servos and posture
        servo.update(current_state);
        servo.setPostureState(brain.getPostureState());
        if (current_state == Hexadrone::DroneState::DRONE_ARMED || servo.isDisarming())
        {
            servo.applyAngles(angles);
        }
        // else: When fully disarmed and the timer has expired, this safely blocks
    }

    // --- 6. Debug/Serial Commands ---
    if (Serial.available())
    {
        static String inputBuffer = ""; // Static keeps the memory alive between loops
        char c = Serial.read();
        // If it's a newline or carriage return, we are done typing
        if (c == '\n' || c == '\r')
        {
            if (inputBuffer.length() > 0)
            {
                inputBuffer.trim();
                if (inputBuffer == "xxwipelogxx")
                    Blackbox.wipeLog();
                else if (inputBuffer == "flush")
                {
                    Blackbox.flushSystem();
                    Blackbox.flushPower();
                }
                else if (inputBuffer == "dump")
                    Blackbox.dumpLog();
                inputBuffer = ""; // Clear the buffer for the next command
            }
        }
        else
        {
            // Add the letter to the buffer
            inputBuffer += c;
        }
    }
}

//
// In main.cpp loop()
// static unsigned long last_servo_update = 0;
// unsigned long now = millis();

// 1. Math is always updated for maximum precision
// std::vector<float> angles = brain.update(dt, input);

// 2. Physical hardware is updated only at the PWM frequency
// if (now - last_servo_update >= 20) { // 20ms = 50Hz
//    last_servo_update = now;
//    servo.applyAngles(angles);
//}
