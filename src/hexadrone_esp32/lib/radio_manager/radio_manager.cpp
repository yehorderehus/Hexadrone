#include "radio_manager.h"

void RadioManager::begin()
{
    // Manually set a much larger RX buffer before calling begin
    Serial2.setRxBufferSize(RADIO_RX_BUF_SIZE);

    Serial2.begin(RADIO_BAUD, SERIAL_8N1, CRSF_RX_PIN, CRSF_TX_PIN);
    _crsf.begin(Serial2);

    Blackbox.logSystem("[RADIO] UART Port Open. Buffer: %d bytes.", RADIO_RX_BUF_SIZE);
}

void RadioManager::update()
{
    _crsf.update();

    bool currentlyConnected = isConnected();

    // If we aren't connected, the buffer might be full of "garbage"
    // from the time the controller was off. Flush it to start fresh.
    if (!currentlyConnected)
    {
        static uint32_t lastFlush = 0;
        if (millis() - lastFlush > RADIO_FLUSH_INTERVAL)
        {
            lastFlush = millis();
            while (Serial2.available() > 0)
            {
                Serial2.read(); // Clear the junk
            }
                }
    }

    if (currentlyConnected && !_wasConnected)
    {
        Blackbox.logSystem("[RADIO] Connection Acquired.");
        _wasConnected = true;
    }
    else if (!currentlyConnected && _wasConnected)
    {
        Blackbox.logSystem("[RADIO] Connection Lost: Force Disarming.");
        _wasConnected = false;
    }
}

void RadioManager::end()
{
    Blackbox.logSystem("[RADIO] UART Port Closed for OTA.");
    Serial2.end(); // Completely shuts down the UART hardware and interrupts
}

int RadioManager::getChannel(int ch) { return _crsf.getChannel(ch); }

bool RadioManager::isConnected()
{
    static unsigned long lastValidPacketTime = 0;

    if (_crsf.isLinkUp())
    {
        lastValidPacketTime = millis();
    }

    // Only declare connection LOST if no packets arrive for 5000ms
    return (millis() - lastValidPacketTime < 5000);
}

int8_t RadioManager::getRSSI()
{
    auto stats = _crsf.getLinkStatistics();
    return stats ? (int8_t)stats->uplink_RSSI_1 : -128; // -128 = "Dead"
}

int RadioManager::getLQ()
{
    return _crsf.getLinkStatistics() ? _crsf.getLinkStatistics()->uplink_Link_quality : 0;
}

// --- Radio translation ---

// Normalize a raw channel value to -1.0 / 0.0 / +1.0
float RadioManager::normalizeStick(int raw_pwm)
{
    // 1. Apply a deadband around the 1500 center.
    // If the stick is within +/- 15 microseconds of center, clamp to 0.0f.
    if (abs(raw_pwm - 1500) < 15)
    {
        return 0.0f;
    }

    // 2. Normalize the remaining signal (-1.0 to 1.0)
    // Calculate based on the original range so max deflection remains 1.0
    float normalized = (float)(raw_pwm - 1500) / 500.0f;

    // Optional but recommended: Clamp to exactly [-1.0, 1.0] in case
    // the radio throws a value slightly outside the 1000-2000 range.
    if (normalized > 1.0f)
        return 1.0f;
    if (normalized < -1.0f)
        return -1.0f;

    return normalized;
}

// Normalize a 3-position switch channel to -1 / 0 / +1
int RadioManager::normalize3Pos(int raw)
{
    if (raw > 1700)
        return 1;
    if (raw < 1300)
        return -1;
    return 0;
}

// Build a ControllerInput from the Radiomaster Pocket CRSF channel values.
// Channel numbering matches the layout in brain.cpp
Hexadrone::ControllerInput RadioManager::buildInput()
{
    Hexadrone::ControllerInput ci{};

    // 1. Primary Sticks [-1.0, 1.0]
    ci.roll = normalizeStick(getChannel(1));     // CH1 (Ail)
    ci.pitch = normalizeStick(getChannel(2));    // CH2 (Ele)
    ci.throttle = normalizeStick(getChannel(3)); // CH3 (Thr)
    ci.yaw = normalizeStick(getChannel(4));      // CH4 (Rud)

    // 2. State & Safety Switches
    ci.armed_switch = (getChannel(5) > 1500) ? 1 : -1; // CH5 (SA)
    ci.posture_switch = normalize3Pos(getChannel(6));  // CH6 (SB)
    ci.gear_switch = normalize3Pos(getChannel(7));     // CH7 (SC)
    ci.oe_kill_button = getChannel(8) > 1500;          // CH8 (SE)

    // 3. Manual Trims ([-1.0, 1.0] for fine-tuning)
    ci.trim_coxa = normalizeStick(getChannel(9));   // CH9
    ci.trim_femur = normalizeStick(getChannel(10)); // CH10
    ci.trim_tibia = normalizeStick(getChannel(11)); // CH11

    // 4. Leg Selector (Digital Trim Mapping)
    int raw_selector = getChannel(12);       // CH12
    ci.btn_prev_leg = (raw_selector < 1300); // Left push
    ci.btn_next_leg = (raw_selector > 1700); // Right push

    return ci;
}

// --- CRSF TELEMETRY DOWNLINK ---

uint8_t RadioManager::crsf_crc8(uint8_t *data, uint8_t len)
{
    // Standard CRSF CRC8 calculation using polynomial 0xD5
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ 0xD5;
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void RadioManager::sendBatteryTelemetry(float voltage, float current, float mah, uint8_t remaining_percent)
{
    // Frame Structure: [Sync] [Length] [Type] [Payload...] [CRC]
    uint8_t frame[12];

    frame[0] = 0xC8; // Sync byte: Flight Controller to Receiver
    frame[1] = 10;   // Length: Type(1) + Payload(8) + CRC(1)
    frame[2] = 0x08; // Frame Type: Battery Sensor

    // Payload 1: Voltage (2 bytes, 0.1V steps, Big-Endian)
    uint16_t v = (uint16_t)(voltage * 10.0f);
    frame[3] = (v >> 8) & 0xFF;
    frame[4] = v & 0xFF;

    // Payload 2: Current (2 bytes, 0.1A steps, Big-Endian)
    uint16_t c = (uint16_t)(current * 10.0f);
    frame[5] = (c >> 8) & 0xFF;
    frame[6] = c & 0xFF;

    // Payload 3: Capacity (3 bytes, mAh, Big-Endian)
    uint32_t cap = (uint32_t)mah;
    frame[7] = (cap >> 16) & 0xFF;
    frame[8] = (cap >> 8) & 0xFF;
    frame[9] = cap & 0xFF;

    // Payload 4: Remaining Battery Percentage (1 byte)
    frame[10] = remaining_percent;

    // Calculate CRC over the Type and Payload bytes (Bytes 2 through 10)
    frame[11] = crsf_crc8(&frame[2], 9);

    // Send the frame out over the UART directly to the Receiver
    Serial2.write(frame, 12);
}
