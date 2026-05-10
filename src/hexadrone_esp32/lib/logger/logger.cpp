#include "logger.h"

Logger Blackbox;

void Logger::begin()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("[LOGGER] LittleFS Mount Failed");
        _fsReady = false;
        return;
    }
    _fsReady = true;
    _fileBuffer.reserve(MAX_BUFFER_SIZE + 256);
    _powerBuffer.reserve(MAX_BUFFER_SIZE + 256);

    if (!LittleFS.exists("/system.log"))
    {
        File f = LittleFS.open("/system.log", FILE_WRITE);
        if (f)
            f.close();
    }

    logSystem("[LOGGER] LittleFS Mounted. Blackbox active.");

    // Ensure CSV header exists
    File csv = LittleFS.open("/power.csv", FILE_READ);
    if (!csv || csv.size() == 0)
    {
        if (csv)
            csv.close();
        File newCsv = LittleFS.open("/power.csv", FILE_APPEND);
        if (newCsv)
        {
            newCsv.println("Timestamp,AvgCell(V),Voltage(V),Current(A),Power(W),Capacity(mAh)");
            newCsv.close();
        }
    }
    else
    {
        csv.close();
    }
}

void Logger::logSystem(const char *format, ...)
{
    char timeBuf[20];
    formatTimestamp(timeBuf, sizeof(timeBuf));

    char msgBuf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(msgBuf, sizeof(msgBuf), format, args);
    va_end(args);

    String finalMsg = String(timeBuf) + " " + String(msgBuf);

    Serial.println(finalMsg); // Live Serial Monitor
    println(finalMsg);        // Send to the RAM buffer
}

void Logger::logPower(float avgCell, float voltage, float current, float power, uint32_t mah)
{
    if (!_fsReady)
        return;

    char timeBuf[20];
    formatTimestamp(timeBuf, sizeof(timeBuf));

    char msgBuf[128];
    snprintf(msgBuf, sizeof(msgBuf), "%s,%.2f,%.2f,%.2f,%.2f,%lu\n", timeBuf, avgCell, voltage, current, power, mah);

    _powerBuffer += String(msgBuf);
    if (_powerBuffer.length() >= MAX_BUFFER_SIZE)
    {
        flushPower();
    }
}

void Logger::flushSystem()
{
    if (!_fsReady || _fileBuffer.length() == 0)
        return;

    File file = LittleFS.open("/system.log", FILE_APPEND);
    if (!file)
    {
        Serial.println("[LOGGER] Failed to open system.log for flush");
        return;
    }

    if (file.size() > MAX_LOG_SIZE)
    {
        file.close();
        LittleFS.remove("/system.log");
        file = LittleFS.open("/system.log", FILE_WRITE);
        if (file)
            file.println("[LOGGER] --- System Log Rotated: Max file size reached ---");
    }

    if (file)
    {
        file.print(_fileBuffer);
        file.close();
        _fileBuffer.clear();
    }
}

void Logger::flushPower()
{
    if (!_fsReady || _powerBuffer.length() == 0)
        return;

    File file = LittleFS.open("/power.csv", FILE_APPEND);
    if (!file)
    {
        Serial.println("[LOGGER] Failed to open power.csv for flush.");
        return;
    }

    if (file.size() > MAX_LOG_SIZE)
    {
        file.close();
        LittleFS.remove("/power.csv");
        file = LittleFS.open("/power.csv", FILE_WRITE);
        if (file)
            file.println("Timestamp,AvgCell(V),Voltage(V),Current(A),Power(W),Capacity(mAh)");
    }

    if (file)
    {
        file.print(_powerBuffer);
        file.close();
        _powerBuffer.clear();
    }
}

void Logger::wipeLog()
{
    if (!_fsReady)
        return;

    LittleFS.remove("/system.log");
    LittleFS.remove("/power.csv");
    _fileBuffer.clear();
    _powerBuffer.clear();

    // 1. Immediately recreate system.log with a header to prevent 0-byte errors
    File sysFile = LittleFS.open("/system.log", FILE_WRITE);
    if (sysFile)
    {
        sysFile.println("[LOGGER] --- Log Reset: All previous data purged ---");
        sysFile.close();
    }

    // 2. Recreate power.csv with the correct header
    File newCsv = LittleFS.open("/power.csv", FILE_WRITE);
    if (newCsv)
    {
        newCsv.println("Timestamp,AvgCell(V),Voltage(V),Current(A),Power(W),Capacity(mAh)");
        newCsv.close();
    }

    Serial.println("[LOGGER] Files wiped and headers recreated.");
}

void Logger::dumpLog()
{
    if (!_fsReady)
    {
        Serial.println("[LOGGER] Cannot dump logs: File system not ready.");
        return;
    }

    // 1. Force flush any pending data in RAM to ensure the dump is perfectly up-to-date
    flushSystem();
    flushPower();

    // 2. Dump the System Log
    Serial.println("\n=== START OF SYSTEM LOG DUMP ===");
    File sysFile = LittleFS.open("/system.log", FILE_READ);
    if (sysFile)
    {
        uint8_t buffer[128]; // Chunk buffer for faster reads
        while (sysFile.available())
        {
            size_t bytesRead = sysFile.read(buffer, sizeof(buffer));
            Serial.write(buffer, bytesRead);
        }
        sysFile.close();
        Serial.println(); // Ensure it ends on a new line
    }
    else
    {
        Serial.println("[LOGGER] Error: Could not open /system.log");
    }
    Serial.println("=== END OF SYSTEM LOG DUMP ===\n");

    // 3. Dump the Power CSV
    Serial.println("=== START OF POWER CSV DUMP ===");
    File pwrFile = LittleFS.open("/power.csv", FILE_READ);
    if (pwrFile)
    {
        uint8_t buffer[128]; // Chunk buffer for faster reads
        while (pwrFile.available())
        {
            size_t bytesRead = pwrFile.read(buffer, sizeof(buffer));
            Serial.write(buffer, bytesRead);
        }
        pwrFile.close();
        Serial.println(); // Ensure it ends on a new line
    }
    else
    {
        Serial.println("[LOGGER] Error: Could not open /power.csv");
    }
    Serial.println("=== END OF POWER CSV DUMP ===\n");
}

void Logger::formatTimestamp(char *buffer, size_t len)
{
    uint32_t ms = millis();
    uint32_t s = ms / 1000;
    uint32_t m = s / 60;
    uint32_t h = m / 60;
    snprintf(buffer, len, "[%02lu:%02lu:%02lu.%03lu]", h, m % 60, s % 60, ms % 1000);
}

void Logger::print(const String &msg)
{
    _fileBuffer += msg;
    if (_fileBuffer.length() >= MAX_BUFFER_SIZE)
    {
        flushSystem();
    }
}

void Logger::println(const String &msg)
{
    print(msg + "\n");
}

void Logger::printf(const char *format, ...)
{
    char loc_buf[256];
    va_list arg;
    va_start(arg, format);
    vsnprintf(loc_buf, sizeof(loc_buf), format, arg);
    va_end(arg);
    print(String(loc_buf));
}
