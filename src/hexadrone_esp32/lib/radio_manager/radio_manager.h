#pragma once
#include <AlfredoCRSF.h>
#include <config.h>
#include <logger.h>
#include <hexadrone_core/state_machine.hpp>

class RadioManager
{
public:
    void begin();
    void update();
    void end();

    int getChannel(int ch);
    bool isConnected();
    int8_t getRSSI();
    int getLQ();

    Hexadrone::ControllerInput buildInput();

    void sendBatteryTelemetry(float voltage, float current, float mah, uint8_t remaining_percent);

private:
    AlfredoCRSF _crsf;

    bool _wasConnected = false;

    float normalizeStick(int raw);
    int normalize3Pos(int raw);

    uint8_t crsf_crc8(uint8_t *data, uint8_t len);
};
