#pragma once

// WiFi Configuration
#define WIFI_SSID "norusko"
#define WIFI_PASS "azovsbs93"

// --- Hardware Mapping ---

// Maps Brain Logical Indices (0-5) to Physical Wiring Blocks (0-7)
//
// Logical: LF=0, RM=1, LB=2, RF=3, LM=4, RB=5
//
// Physical Blocks (4-pin spacing):
// Board 1 (0x40): Block 0 (Pins 0-2), Block 1 (Pins 4-6), Block 2 (Pins 8-10), Block 3 (Pins 12-14)
// Board 2 (0x41): Block 4 (Pins 0-2), Block 5 (Pins 4-6), Block 6 (Pins 8-10), Block 7 (Pins 12-14)

constexpr int LOGICAL_TO_PHYSICAL[6] = {
    1, // Logical 0 (LF) -> Board 1 (0x40), Pins 4-6   (Block 1)
    0, // Logical 1 (RM) -> Board 1 (0x40), Pins 0-2   (Block 0)
    4, // Logical 2 (LB) -> Board 2 (0x41), Pins 0-2   (Block 4)
    5, // Logical 3 (RF) -> Board 2 (0x41), Pins 4-6   (Block 5)
    6, // Logical 4 (LM) -> Board 2 (0x41), Pins 8-10  (Block 6)
    7  // Logical 5 (RB) -> Board 2 (0x41), Pins 12-14 (Block 7)
};

// Servo Direction Calibration
// 1.0f = Normal, -1.0f = Inverted
// Order: [Coxa, Femur, Tibia] for each leg (0-5)
// Only change to -1.0f AFTER mechanical alignment if a leg still moves backwards.
constexpr float SERVO_SIGNS[18] = {
    // Group A (LF, RM, LB)
    1.0f, 1.0f, 1.0f, // Leg 0 (LF)
    1.0f, 1.0f, 1.0f, // Leg 1 (RM)
    1.0f, 1.0f, 1.0f, // Leg 2 (LB)

    // Group B (RF, LM, RB)
    1.0f, 1.0f, 1.0f, // Leg 3 (RF)
    1.0f, 1.0f, 1.0f, // Leg 4 (LM)
    1.0f, 1.0f, 1.0f  // Leg 5 (RB)
};

// I2C Pins
#define SDA_CUSTOM 25
#define SCL_CUSTOM 33
#define OE_CUSTOM 23

// Radio Pins
#define CRSF_RX_PIN 27
#define CRSF_TX_PIN 26

// I2C Addresses
#define ADDR_SERVO_1 0x40
#define ADDR_SERVO_2 0x41
#define ADDR_POWER 0x45

// --- Cutoff & Warning Values ---

// Battery voltage thresholds (6S Li-Ion)
#define FULL_VOLTAGE 24.6f        // 4.1V per cell, 100% charged
#define WARNING_VOLTAGE 20.4f     // 3.4V per cell, warning level
#define SOFT_CUTOFF_VOLTAGE 19.8f // 3.3V per cell, soft cutoff
#define HARD_CUTOFF_VOLTAGE 18.0f // 3.0V per cell, hard cutoff
#define IGNORE_VOLTAGE 6.0f       // Ignore readings below this

// Power warning threshold
#define WARNING_WATTAGE 150.0f // W, high power warning

// Timing intervals (ms)
#define VOLTAGE_WARNING_INTERVAL 5000 // Delay before voltage warning triggers
#define SOFT_CUTOFF_INTERVAL 2000     // Delay before soft cutoff triggers
#define HARD_CUTOFF_INTERVAL 500      // Delay before hard cutoff triggers
#define WATTAGE_WARNING_INTERVAL 500  // Delay before wattage warning triggers
#define DISARM_TRIGGER_INTERVAL 2000  // Time to fully disarm servos
#define SERVO_ARM_INTERVAL 100        // Time between sequential servo power-up/down steps

// --- Software mapping ---

// Logging & Blackbox Configuration
#define BLACKBOX_BUFFER_SIZE 1024    // RAM buffer before flushing (bytes)
#define BLACKBOX_MAX_LOG_SIZE 102400 // 100 KB safety limit per file
#define LOG_VOLTAGE_THRESH 0.2f
#define LOG_CURRENT_THRESH 0.2f

// Baud rates
#define SERIAL_BAUD 115200
#define RADIO_BAUD 420000

// Radio Performance Tuning
#define TELEMETRY_INTERVAL 100   // ms <-> 10Hz
#define RADIO_FLUSH_INTERVAL 500 // ms
#define RADIO_RX_BUF_SIZE 2048   // Buffer for high-speed CRSF packets (bytes)

// Servo PWM Ticks (Standard 0.5ms - 2.5ms range)
#define PWM_FREQ 50  // Hz
#define SERVOMIN 102 // The 0 degree limit
#define SERVOMAX 512 // The 180 degree limit
