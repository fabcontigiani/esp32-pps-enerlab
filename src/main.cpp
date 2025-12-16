#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "AdvancedLogger.h"

const char *customLogPath = "/logs.txt";

const int timeZone = -3 * 3600000; // UTC. In milliseconds
const int daylightOffset = 0; // No daylight saving time. In milliseconds
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const char *ntpServer3 = "time.windows.com";

long lastMillisLogClear = 0;
const long intervalLogClear = 30000;

char api_token[120];

// WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
WiFiManager wm;

WiFiManagerParameter custom_api_token("api_token", "API Token", api_token, sizeof(api_token));

void saveWifiCallback()
{
    Serial.println("[CALLBACK] saveCallback fired");
}

// gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager)
{
    Serial.println("[CALLBACK] configModeCallback fired");
    // myWiFiManager->setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
    // Serial.println(WiFi.softAPIP());
    // if you used auto generated SSID, print it
    // Serial.println(myWiFiManager->getConfigPortalSSID());
    //
    // esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
}

void handleLogsRoute()
{
    Serial.println("[HTTP] handle logs route");
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

void setup()
{
    // Initialize Serial and LittleFS (mandatory for the AdvancedLogger library)
    // --------------------
    Serial.begin(115200);

    if (!LittleFS.begin(true)) // Setting to true will format the LittleFS if mounting fails
    {
        Serial.println("An Error has occurred while mounting LittleFS"); // TODO: handle error
    }

    AdvancedLogger::begin(customLogPath);

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

    wm.addParameter(&custom_api_token);

    // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    wm.resetSettings();

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    if (!wm.autoConnect("Enerlab WiFiManager", "password"))
    {
        Serial.println("Failed to connect");
        // ESP.restart();
    }
    else
    {
        // if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");
    }

    LOG_INFO(("IP address: " + WiFi.localIP().toString()).c_str());

    configTime(timeZone, daylightOffset, ntpServer1, ntpServer2, ntpServer3);

    wm.startWebPortal(); // Post connection WiFi Manager Portal Start

    LOG_DEBUG("Server started!");

    LOG_INFO("Setup done!");
}

void loop()
{
    // put your main code here, to run repeatedly:
    wm.process();

    // LOG_DEBUG("This is a debug message!");
    // delay(500);
    // LOG_INFO("This is an info message!!");
    // delay(500);
    // LOG_WARNING("This is a warning message!!!");
    // delay(500);
    // LOG_ERROR("This is a error message!!!!");
    // delay(500);
    // LOG_FATAL("This is a fatal message!!!!!");
    // delay(500);
    // LOG_INFO("This is an info message!!", true);
    // delay(1000);

    // if (millis() - lastMillisLogClear > intervalLogClear)
    // {
    //     LOG_INFO("Current number of log lines: %d", AdvancedLogger::getLogLines());
    //     AdvancedLogger::clearLog();
    //     AdvancedLogger::setDefaultConfig();
    //     LOG_WARNING("Log cleared!");

    //     lastMillisLogClear = millis();
    // }
}
