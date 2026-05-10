#include "comms_manager.h"
#include "radio_manager.h"

extern RadioManager radio;

void CommsManager::begin(const char *ssid, const char *password)
{
    // --- WiFi Credentials ---
    _ssid = String(ssid);
    _password = String(password);

    // --- Wireless updates (ArduinoOTA) ---
    ArduinoOTA.onStart([this]()
                       {
                        Serial.println("\n[OTA] Update Starting.");
                        _otaLockdown = true;
                        Blackbox.logSystem("[COMMS] OTA lockdown engaged: freezing legs and skipping sensors.");
                        radio.end();
                        Blackbox.logSystem("[COMMS] Web server terminated to prioritize OTA bandwidth.");
                        _server.end(); });
    ArduinoOTA.onEnd([this]()
                     {
                         Serial.println("\n[OTA] Update Complete.");
                         _otaLockdown = false;
                         Blackbox.logSystem("[COMMS] OTA lockdown released. Update successful.");
                         Blackbox.flushSystem();
                         Blackbox.flushPower();
                         delay(500); // Give the Flash chip a half-second to finish writing
                     });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.onError([this](ota_error_t error)
                       {
                        Serial.printf("[OTA] Error[%u]\n", error);
                        _otaLockdown = false;
                        Blackbox.logSystem("[COMMS] OTA failed with Error[%u]. Forcing hardware reboot...", error);
                        Blackbox.flushSystem(); 
                        Blackbox.flushPower();
                        delay(500); // Give the Flash chip a half-second to finish writing
                        ESP.restart(); });

    // --- Web Server Routes ---

    // 1. Download System Log
    _server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    if (!LittleFS.exists("/system.log")) {
        request->send(200, "text/plain", "Log file not initialized yet. Reboot or trigger an event.");
        return;
    }
    
    File file = LittleFS.open("/system.log", "r");
    if (file && file.size() > 0) {
        file.close();
        request->send(LittleFS, "/system.log", "text/plain", true); 
    } else {
        if(file) file.close();
        request->send(200, "text/plain", "Log is currently empty or locked.");
    } });

    // 2. Download Power CSV
    _server.on("/telemetry", HTTP_GET, [](AsyncWebServerRequest *request)
               { 
    if (LittleFS.exists("/power.csv")) {
        // Removed the flush commands!
        request->send(LittleFS, "/power.csv", "text/csv", true); 
    } else {
        request->send(200, "text/plain", "Telemetry data not available yet.");
    } });

    // 3. Wipe Logs Route
    _server.on("/wipe", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        Blackbox.wipeLog();
        // Redirect back to the main terminal page after wiping
        request->send(200, "text/html", "<html><body><h2>Logs Purged.</h2><br><a href='/'>Return to Terminal</a></body></html>"); });

    // 4. The Terminal UI
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        String html = "<html><head><title>Gunslinger Terminal</title>";
        html += "<style>body{background:#0a0a0a; color:#ff4444; font-family:monospace; padding:20px;}";
        html += ".btn{display:block; width:200px; padding:10px; margin:10px 0; background:#222; border:1px solid #ff4444; color:#ff4444; text-decoration:none; text-align:center;}";
        html += ".btn:hover{background:#ff4444; color:#000;}</style></head><body>";
        html += "<h1>UNIT: M1A1-GUNSLINGER</h1>";
        html += "<h3>-- Maintenance Protocol --</h3>";
        html += "<a class='btn' href='/logs' download='system.log'>DOWNLOAD SYSTEM LOG</a>";
        html += "<a class='btn' href='/telemetry' download='power.csv'>DOWNLOAD POWER CSV</a>";
        html += "<a class='btn' href='/wipe' onclick=\"return confirm('Purge all mission data?')\">WIPE ALL LOGS</a>";
        html += "</body></html>";
        
        request->send(200, "text/html", html); });
}

void CommsManager::update(Hexadrone::DroneState currentState)
{
    static unsigned long lastToggleTime = 0;
    const unsigned long SETTLE_INTERVAL = 2000; // 2 seconds safety window

    // 1. STATE MACHINE ALERTS
    if (currentState != _lastDroneState && (millis() - lastToggleTime > SETTLE_INTERVAL))
    {
        lastToggleTime = millis();

        // Emergency Override
        if (currentState == Hexadrone::DroneState::DRONE_OE_KILLED)
        {
            if (!_wifiEnabled)
            {
                Blackbox.logSystem("[COMMS] OE-KILL detected: Enabling WiFi for emergency diagnostics.");
                enableWiFi(_ssid.c_str(), _password.c_str());
            }
        }
        // Normal State Changes
        else
        {
            if (currentState == Hexadrone::DroneState::DRONE_ARMED)
            {
                Blackbox.logSystem("[COMMS] System ARMED: Suppressing WiFi for Radio performance.");
                disableWiFi();
            }
            else if (currentState == Hexadrone::DroneState::DRONE_DISARMED)
            {
                Blackbox.logSystem("[COMMS] System DISARMED: Booting WiFi for Diagnostics.");
                enableWiFi(_ssid.c_str(), _password.c_str());
                Blackbox.flushSystem();
                Blackbox.flushPower();
            }
        }
        _lastDroneState = currentState;
    }

    // 2. NETWORK MANAGEMENT
    if (!_wifiEnabled)
        return;

    if (_state == CommsState::CONNECTING)
    {
        // Check if we are connected AND have a real IP address
        if (WiFi.status() == WL_CONNECTED && WiFi.localIP().toString() != "0.0.0.0")
        {
            _state = CommsState::CONNECTED;

            if (!_serversStarted)
            {
                _server.begin();
                ArduinoOTA.begin();
                _serversStarted = true;

                if (MDNS.begin("hexadrone"))
                {
                    // 1. Advertise the Web Server so PCs/Phones can "see" it
                    MDNS.addService("http", "tcp", 80);

                    // 2. Announce the name to the network immediately
                    Blackbox.logSystem("[COMMS] mDNS active: http://hexadrone.local");
                }
                else
                {
                    Blackbox.logSystem("[COMMS] mDNS Failed to start.");
                }
            }
        }
    }

    else if (_state == CommsState::CONNECTED)
    {
        ArduinoOTA.handle();

        // If we lose connection, don't just 'reconnect'—restart the whole attempt
        if (WiFi.status() != WL_CONNECTED)
        {
            _state = CommsState::CONNECTING;
            Blackbox.logSystem("[COMMS] Connection lost. Re-initializing...");
            WiFi.begin(_ssid.c_str(), _password.c_str()); // Hard restart of the connection
        }
    }

    else if (_state == CommsState::CONNECTED)
    {
        ArduinoOTA.handle();

        if (WiFi.status() != WL_CONNECTED)
        {
            _state = CommsState::CONNECTING;
            Blackbox.logSystem("[COMMS] WiFi Signal lost. Attempting to reconnect...");
            WiFi.disconnect();
            WiFi.reconnect();
        }
    }
}

void CommsManager::enableWiFi(const char *ssid, const char *password)
{
    if (!_wifiEnabled)
    {
        Blackbox.logSystem("[COMMS] Initializing WiFi for %s...", ssid);

        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false);
        WiFi.begin(ssid, password); // Start the connection

        _wifiEnabled = true;
        _state = CommsState::CONNECTING;
        _lastDotTime = millis();
    }
}

void CommsManager::disableWiFi()
{
    if (_wifiEnabled)
    {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);

        _wifiEnabled = false;
        _state = CommsState::DISCONNECTED;

        Blackbox.logSystem("[COMMS] WiFi Disabled (Radio Safe Mode).");
    }
}
