#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "AdvancedLogger.h"

// Let's Encrypt ISRG Root X1 certificate (valid until 2035)
const char* letsencrypt_root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"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\n" \
"-----END CERTIFICATE-----\n";

const char *customLogPath = "/logs.txt";

// UART Configuration
#define RXD2 16  // GPIO16 for UART RX
#define TXD2 17  // GPIO17 for UART TX
#define UART_BAUD 115000
HardwareSerial uartSerial(2);  // Use UART2

const int timeZone = -3 * 3600000; // UTC. In milliseconds
const int daylightOffset = 0; // No daylight saving time. In milliseconds
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const char *ntpServer3 = "time.windows.com";

const unsigned long minFreeFlash = 50000; // Clear logs if free flash drops below 50KB
long lastMillisMqttReconnect = 0;
const long intervalMqttReconnect = 5000;
bool mqttWasConnected = true; // Track MQTT connection state for logging

char mqtt_server[40] = "www.enerlab.duckdns.org";
char mqtt_port[6] = "8883";
char mqtt_user[40] = "";
char mqtt_password[40] = "";
char mqtt_topic[60] = "";

// MQTT Client with TLS
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
WiFiManager wm;

WiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT Server", mqtt_server, sizeof(mqtt_server));
WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT Port", mqtt_port, sizeof(mqtt_port));
WiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT Username", mqtt_user, sizeof(mqtt_user));
WiFiManagerParameter custom_mqtt_password("mqtt_password", "MQTT Password", mqtt_password, sizeof(mqtt_password));
WiFiManagerParameter custom_mqtt_topic("mqtt_topic", "MQTT Topic", mqtt_topic, sizeof(mqtt_topic));

void saveWifiCallback()
{
    LOG_DEBUG("[CALLBACK] saveCallback fired");
}

// gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager)
{
    LOG_DEBUG("[CALLBACK] configModeCallback fired");
    // myWiFiManager->setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
    // Serial.println(WiFi.softAPIP());
    // if you used auto generated SSID, print it
    // Serial.println(myWiFiManager->getConfigPortalSSID());
    //
    // esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
}

void handleLogsRoute()
{
    LOG_DEBUG("[HTTP] handle logs route");
    // Serve the log file stored in LittleFS (customLogPath = "/logs.txt")
    File f = LittleFS.open(customLogPath, "r");
    if (!f)
    {
        wm.server->send(404, "text/plain", "Log file not found");
        return;
    }

    String content;
    content.reserve((size_t)f.size());
    while (f.available())
    {
        content += (char)f.read();
    }
    f.close();

    wm.server->send(200, "text/plain", content);
}

void bindServerCallback()
{
    wm.server->on("/logs", handleLogsRoute); // this is now crashing esp32 for some reason
                                           // wm.server->on("/info",handleRoute); // you can override wm!
}

bool reconnectMQTT()
{
    if (mqttClient.connected())
    {
        return true;
    }
    
    String clientId = "Enerlab-" + String(WiFi.macAddress());
    
    // Attempt to connect
    bool connected = false;
    if (strlen(mqtt_user) > 0)
    {
        connected = mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_password);
    }
    else
    {
        connected = mqttClient.connect(clientId.c_str());
    }
    
    if (connected)
    {
        if (!mqttWasConnected)
        {
            LOG_INFO("MQTT reconnected successfully");
            mqttWasConnected = true;
        }
        return true;
    }
    else
    {
        if (mqttWasConnected)
        {
            LOG_ERROR("MQTT connection failed, rc=%d. Will retry in %ld seconds", mqttClient.state(), intervalMqttReconnect / 1000);
            mqttWasConnected = false;
        }
        return false;
    }
}

void setup()
{
    // Initialize Serial and LittleFS (mandatory for the AdvancedLogger library)
    // --------------------
    Serial.begin(115200);

    if (!LittleFS.begin(true)) // Setting to true will format the LittleFS if mounting fails
    {
        LOG_ERROR("An Error has occurred while mounting LittleFS");
    }

    AdvancedLogger::begin(customLogPath);
    
    // Configure log levels: DEBUG and above to Serial, WARNING and above to file
    AdvancedLogger::setPrintLevel(LogLevel::DEBUG);
    AdvancedLogger::setSaveLevel(LogLevel::INFO);

    LOG_DEBUG("AdvancedLogger setup done!");

    wm.setHostname("enerlab");

    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    // it is a good practice to make sure your code sets wifi mode how you want it.

    WiFi.setTxPower(WIFI_POWER_8_5dBm); // ESP32-C3 Supermini specific tweak

    // put your setup code here, to run once:
    Serial.begin(115200);

    wm.setWebServerCallback(bindServerCallback);
    const char* menuhtml = "<form action='/logs' method='get'><button>Logs</button></form><br/>\n";
    wm.setCustomMenuHTML(menuhtml);
    // std::vector<const char *> menu = {"wifi", "param", "sep", "info", "custom", "sep", "update", "restart", "exit"};
    std::vector<const char *> menu = {"wifi", "param", "sep", "info", "custom", "sep", "restart"};
    wm.setMenu(menu);

    const char *headerhtml = "<img alt='logo' width='300' height='80' src='data:image/png;base64,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' />";
    wm.setCustomBodyHeader(headerhtml);
    // wm.setCustomBodyFooter("<h1>THIS IS MY FOOTER</h1>");
    const char *headhtml = "<link rel='icon' type='image/png' href='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAADhElEQVR42rWXTWtcdRSHfwxhKBJCGERciEsppQsREZEikpULEZEuRLoQF1l0UbqQklVBRPwEfgCRLlxIF7bq2MaQ0tqaSTIdk3QySW4nNzczk1IkDBKGEOYcH5pLb8NkYF5uDjzMC8yc555z5n/PqN/wsjK2rI98SYGX5F6EBSjA32rbA13y+8roJMIrytqqPrUyyVdIuAz/wENYlNu8nlpBH+okwgKdsg1N2roiq5BwFR7BCixBCYGHKtiiTivt8FDjXtWUP1bLA5Ktw1osUY5FlmFJ16jIKaUZtqWcb+k721TTN0nyGALYiEXiatCWfXukqfQS12Fbr8BPHmkfCfcQNqHaKWJraiLyXjolbyiDwFmr6WevkWAbIoBuIsxG1SoaVRphDZ1BYNbrXDkCsQR0SCQiga4Pn3hHWZhAIPQduTdi6gCJTKcIMzI5XNmfaITkn/FYeZYcepGIRdoW6tyA/Ya6siT7Ap5YnNgOE7fhwMGAthwcL4JApLc0SPDBHJP+NV9SoZT/0c92PFj7THjTNtTgMeC9wKo8r/L+plpc9S6f+9dqh3PC828sUnYQgcskbz7vZzLZyYFT6ThwWrBrywisiN+/WramIlzm9ZuQUa+BeTXpZc8SyT1gAeap2hwteoDYPcTuaMFm9L1N64Ld0mnL61X7rUt1KF877uNgEkVYVHw3hPtwF2ZhRm639dT+0LT9rqt2U+/YDY0cFair2DHVYe8SyS25i8SfcBuRvA6oQgmJCb9xVOBtq+kmrNOOXUTaA7djEeZhLpa497wSSEBejkTeftXYkTMfcgi8S/LPkZiySD/alkpItNJsR1yJAHLqFrQjg8Q4Eq8j8b6F+gX2+h7MAswdKzHjt6hAr0EVRuEKlWj2346jlbBZBnJGF2y6z5UNgXELlR+yHXt2V1/5HY0Nuphc7GEw4xUtqYQtQEENJL70v5TVoGGRznsk71fCiiozE+chq2GCwbzY1zmBhC2pZiWdox0jGia8DmxHHUtJFwmr6MDKmmYmzqa1H76MwAp4L8e2rem6reoMEkprRZtAouEIdDu27VBi3wJdQ+I1pRW+IyFwBVrJMtK5I1oodgl9CzmlGbajUfghWc061zKLtGdbuorEmIi0Bd6gCgWSH7sf2rZCBCYReEnESQh8AM0uS2poNX0MIzqpYEu+ZHHyFyWsrgp8YrXh/or/D/l1zQWdox4NAAAAAElFTkSuQmCC' />";
    wm.setCustomHeadElement(headhtml);

    wm.addParameter(&custom_mqtt_server);
    wm.addParameter(&custom_mqtt_port);
    wm.addParameter(&custom_mqtt_user);
    wm.addParameter(&custom_mqtt_password);
    wm.addParameter(&custom_mqtt_topic);

    // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    // wm.resetSettings();

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    if (!wm.autoConnect("Enerlab WiFiManager", "password"))
    {
        LOG_ERROR("Failed to connect");
        // ESP.restart();
    }
    else
    {
        // if you get here you have connected to the WiFi
        LOG_INFO("WiFi connected successfully");
    }

    LOG_INFO(("IP address: " + WiFi.localIP().toString()).c_str());

    // Generate MQTT topic with MAC address if not configured
    if (strlen(mqtt_topic) == 0)
    {
        String mac = WiFi.macAddress();
        mac.replace(":", "");
        snprintf(mqtt_topic, sizeof(mqtt_topic), "enerlab/sensor/%s", mac.c_str());
    }
    LOG_INFO(("MQTT Topic: " + String(mqtt_topic)).c_str());

    configTime(timeZone, daylightOffset, ntpServer1, ntpServer2, ntpServer3);

    // Initialize UART2 for sensor communication
    uartSerial.begin(UART_BAUD, SERIAL_8N1, RXD2, TXD2);
    LOG_INFO("UART initialized on RX=%d, TX=%d, Baud=%d", RXD2, TXD2, UART_BAUD);

    // Initialize MQTT with TLS
    if (strlen(mqtt_server) > 0)
    {
        // Configure TLS with Let's Encrypt root CA certificate
        espClient.setCACert(letsencrypt_root_ca);
        
        mqttClient.setServer(mqtt_server, atoi(mqtt_port));
        LOG_INFO("MQTTS configured: %s:%s", mqtt_server, mqtt_port);
        
        // Try initial connection but don't block if it fails
        reconnectMQTT();
    }
    else
    {
        LOG_WARNING("MQTT server not configured");
    }

    wm.startWebPortal(); // Post connection WiFi Manager Portal Start

    LOG_DEBUG("Server started!");

    LOG_INFO("Setup done!");
}

void loop()
{
    // put your main code here, to run repeatedly:
    wm.process();

    // Check flash storage and clear logs if needed
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    size_t freeFlash = totalBytes - usedBytes;
    
    if (freeFlash < minFreeFlash)
    {
        LOG_INFO("Low flash storage detected (%d bytes free), clearing old log entries", freeFlash);
        AdvancedLogger::clearLogKeepLatestXPercent(50);
    }

    // Ensure MQTT stays connected with timed retry
    if (strlen(mqtt_server) > 0)
    {
        if (!mqttClient.connected())
        {
            long currentMillis = millis();
            if (currentMillis - lastMillisMqttReconnect >= intervalMqttReconnect)
            {
                lastMillisMqttReconnect = currentMillis;
                reconnectMQTT();
            }
        }
        else
        {
            mqttClient.loop();
        }
    }

    // Read sensor data from UART
    if (uartSerial.available())
    {
        String uartData = uartSerial.readStringUntil('\n');
        uartData.trim();
        
        if (uartData.length() > 0)
        {
            LOG_DEBUG(("UART received: " + uartData).c_str());
            
            // Parse the received data as JSON
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, uartData);
            
            if (error)
            {
                LOG_WARNING("JSON parse failed: %s", error.c_str());
                
                // If raw data is not JSON, create a JSON message with the raw data
                doc.clear();
                doc["raw_data"] = uartData;
                doc["timestamp"] = millis();
            }
            
            // Serialize JSON and publish to MQTT
            String jsonOutput;
            serializeJson(doc, jsonOutput);
            
            if (mqttClient.connected())
            {
                if (mqttClient.publish(mqtt_topic, jsonOutput.c_str()))
                {
                    LOG_INFO(("Published to MQTT: " + jsonOutput).c_str());
                }
                else
                {
                    LOG_ERROR("MQTT publish failed");
                }
            }
            else
            {
                LOG_WARNING("MQTT not connected, skipping publish");
            }
        }
    }
}