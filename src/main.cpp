#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <EEPROM.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "data.h"

const String appTitle = "ESP8266 WiFi";
const String hostname = "esp8266.local.cloud";

String configErrorMessage;
String configNetworksOptions;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

IPAddress configHotSpotIpAddress(192, 168, 24, 1);
IPAddress configHotSpotNetmask(255, 255, 255, 0);

void notifyClients() {
  ws.textAll(String("ledState"));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) {
      //ledState = !ledState;
      notifyClients();
    }
  }
}

/**
 * 
 */
void onEvent(
    AsyncWebSocket *server, 
    AsyncWebSocketClient *client, 
    AwsEventType type,
    void *arg, 
    uint8_t *data, 
    size_t len
) {
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

/**
 * 
 */
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

/**
 * 
 */
String processor(const String& var) {
  Serial.println(var);
  if(var == "STATE"){
    if (ledState){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
  return String();
}

/**
 * Get IP address assigned by DHCP to the device.
 */
String getClientIpAddress(void) {
    IPAddress ipAddress = WiFi.localIP();
    return String(ipAddress[0]) + '.' + String(ipAddress[1]) + '.' + String(ipAddress[2]) + '.' + String(ipAddress[3]);
}

/**
 * Test WiFi status for connection.
 */
bool testWifi(void) {
    int c = 0;
    Serial.println("Waiting for WiFi to connect...");
    while (c < 20) {
        if (WiFi.status() == WL_CONNECTED) {
            return true;
        }
        delay(500);
        Serial.print(".");
        c++;
    }
    Serial.println("");
    Serial.println("Connection timed out.");
    return false;
}

/**
 * Scan networks and prepare the list for the login form.
 */
void configScanNetworks(void) {
    int countNetworks = WiFi.scanNetworks();
    Serial.println("Networks scan completed.");
    if (countNetworks == 0) {
        Serial.println("No WiFi Networks found");
        configNetworksOptions = "<option value=0>No networks found</option>";
        configNetworksOptions += "<option value=-1>Scan for networks</option>";
    } else {
        Serial.print(countNetworks);
        Serial.println(" Networks found");
        configNetworksOptions = "<option value=0>Select a network</option>";
        for (int i = 0; i < countNetworks; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
            delay(10);
            configNetworksOptions += "<option value=\"" + WiFi.SSID(i) + "\">";
            configNetworksOptions += WiFi.SSID(i);
            configNetworksOptions += " (";
            configNetworksOptions += WiFi.RSSI(i);
            configNetworksOptions += ")";
            configNetworksOptions += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
            configNetworksOptions += "</option>";
        }
    }
}

/**
 * Setup the HotSpot to access on config area.
 */
void configHotSpotSetup(void) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect();
    delay(100);
    WiFi.softAPConfig(configHotSpotIpAddress, configHotSpotIpAddress, configHotSpotNetmask);
    delay(100);
    configScanNetworks();
    delay(100);
    WiFi.softAP(appTitle + " " + ESP.getChipId(), "");
}

/**
 *
 */
String configFormHtml(void) {
    String configFormHtml = "<form id=config> SSID <select id=network name=ssid> "+ configNetworksOptions +" </select> Password <input type=password name=passphrase required> <button id=connect type=button>Connect</button></form>";
    return configFormHtml;
}

/**
 *
 */
void defaultWebServerRegisterRoutes(void) {
    server.on("/welcome", HTTP_GET, [](AsyncWebServerRequest *request) {
        String welcomeHtml = "<h1>Welcome</h1>";
        server.send_P(200, "text/html", welcomeHtml);
    });
    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
        dataErase(0, 96);
        dataCommit();
        server.send_P(200, "text/html", "<h1>Reset ok!</h1>");
        delay(500);
        ESP.reset();
    });
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->header("Location", "/welcome", true);
        webServer.send(302, "text/plane", "");
    });    
}

/**
 *
 */
void configWebServerRegisterRoutes(void) {
    Serial.println("Register config web server routes");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        // @inject "../app/config/index.html"
        String configIndexHtml PROGMEM = "<!DOCTYPE html><html lang=en><meta charset=UTF-8><title>" + appTitle + "</title><meta name=viewport content=\"width=device-width,initial-scale=1\"><link rel=\"shortcut icon\" type=image/x-icon href=data:image/x-icon;,><style>" + styleCss + "</style><body><h1>" + appTitle + "</h1><div id=page></div><div id=mask></div><a href=http://javanile.org target=_blank>Javanile.org</a><script>" + appJs + ";" + configAppJs + ";</script></body></html>";,><style>" + styleCss + "</style><body><h1>" + appTitle + "</h1><div id=page></div><div id=mask></div><a href=http://javanile.org target=_blank>Javanile.org</a><script>    
        server.send_P(200, "text/html", configIndexHtml);
    });
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        request.send(200, "text/html", configFormHtml());
    });
    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        configScanNetworks();
        request.send(200, "text/html", configFormHtml());
    });
    server.on("/connect", HTTP_GET, [](AsyncWebServerRequest *request) {
        int statusCode;
        bool validData = false;
        String ssid = request->arg("ssid");
        String passphrase = request->arg("passphrase");
        String content;
        if (ssid.length() > 0 && passphrase.length() > 0) {
            dataErase(0, 96);
            dataSaveAsString(0, ssid);
            dataSaveAsString(32, passphrase);
            dataCommit();
            WiFi.begin(ssid.c_str(), passphrase.c_str());
            if (testWifi()) {
                String clientIpAddress = getClientIpAddress();
                content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}" + clientIpAddress;
                statusCode = 200;
                validData = true;
            } else {
                content = "{\"Error\":\"404 not found\"}";
                statusCode = 404;
            }
        } else {
            content = "{\"Error\":\"404 not found\"}";
            statusCode = 404;
        }
        request->send(statusCode, "application/json", content);
        if (validData) {
            delay(500);
            ESP.reset();
        }
    });
}

/**
 *
 */
void discoverServerStart(void) {
    /*
    discoverServer.on("/_discover", []() {
        String discoverInfo = "{\"name\":\"" + appTitle + "\"}";
        discoverServer.sendHeader("Access-Control-Allow-Origin", "*");
        discoverServer.sendHeader("Access-Control-Allow-Methods", "*");
        discoverServer.send(200, "application/json", discoverInfo);
    });
    discoverServer.onNotFound([]() {
        discoverServer.send(403, "text/html", "<h1>Forbidden</h1>");
    });
    discoverServer.begin();
    */
}

/**
 * System bootstrap.
 */
void setup(void) {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    WiFi.hostname(hostname);
    delay(100);

    Serial.println("Disconnecting previously connected WiFi");
    WiFi.disconnect();
    delay(200);

    Serial.println("Reading SSID and passphrase from EEPROM");
    EEPROM.begin(512);
    delay(10);
    String ssid = dataReadAsString(0, 32);
    String passphrase = dataReadAsString(32, 96);
    Serial.println("- SSID: " + ssid);
    Serial.println("- Passphrase: " + passphrase);

    Serial.println("Perform WiFi connection with EEPROM");
        WiFi.begin(ssid.c_str(), passphrase.c_str());
        if (testWifi()) {
        Serial.println("Successfully connected.");
        //discoverServerStart();
        defaultWebServerRegisterRoutes();
        initWebSocket();
        server.begin();
        return;
    }

    Serial.println("Turning on the config HotSpot.");
    discoverServerStart();
    configWebServerRegisterRoutes();
    //webServer.begin();
    initWebSocket();
    server.begin();
    configHotSpotSetup();
    /*while ((WiFi.status() != WL_CONNECTED)) {
        delay(100);
        webServer.handleClient();
        discoverServer.handleClient();
    }*/
}

/**
 * System main loop.
 */
void loop(void) {
    if ((WiFi.status() == WL_CONNECTED)) {        
        delay(100);
        //webServer.handleClient();
        //discoverServer.handleClient();
        
        ws.cleanupClients();
    }
}

