#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <cstdarg>
#include <config.h>

class Logger
{
public:
    void begin();

    void logSystem(const char *format, ...);
    void logPower(float avgCell, float voltage, float current, float power, uint32_t mah);

    void flushSystem();
    void flushPower();
    void wipeLog();
    void dumpLog();

private:
    void formatTimestamp(char *buffer, size_t len);

    void print(const String &msg);
    void println(const String &msg);
    void printf(const char *format, ...);

    bool _fsReady = false;

    String _fileBuffer;
    String _powerBuffer;
    const unsigned int MAX_BUFFER_SIZE = BLACKBOX_BUFFER_SIZE;
    const unsigned int MAX_LOG_SIZE = BLACKBOX_MAX_LOG_SIZE;
};

extern Logger Blackbox;
