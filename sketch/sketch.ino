#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WebSocketsServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

#include <ArduinoJson.h>
#include <WiFiManager.h>       // circa September 2019 development branch - https://github.com/tzapu/WiFiManager.git

#include "auth.h"
#include "html.h"

MDNSResponder mdns;
WebSocketsServer webSocket = WebSocketsServer(81);
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

// parameters
char device_name[40];
char hostname[18];
bool resetRequired = false;
unsigned long loopLastRun;

const char* update_path = "/firmware";
const char* update_username = AUTH_USERNAME;
const char* update_password = AUTH_PASSWORD;

// Pins
const int POWER_PIN = 2;
const int SPEED_LOW_PIN = 0;
const int SPEED_MED_PIN = 4;
const int SPEED_MAX_PIN = 5;

#define STATE_OFF       HIGH
#define STATE_ON        LOW

String currentState = "off";
int currentSpeed = 0;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      Serial.printf("[%u] Connected from url: %s\r\n", num, payload);
      sendUpdate();
      break;
    case WStype_TEXT: {
      Serial.printf("[%u] get Text: %s\r\n", num, payload);

      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject((char *)&payload[0]);

      if (root.containsKey("power")) {
        togglePower(root["power"]);
      }

      if (root.containsKey("speed")) {
        currentSpeed = root["speed"];
        adjustSpeed(root["speed"]);
      }

      break;
    }
    case WStype_PING:
      // Serial.printf("[%u] Got Ping!\r\n", num);
      break;
    case WStype_PONG:
      // Serial.printf("[%u] Got Pong!\r\n", num);
      break;
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      break;
    default:
      Serial.printf("Invalid WStype [%d]\r\n", type);
      break;
  }
}

void sendUpdate() {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["speed"] = currentSpeed;

  if (digitalRead(POWER_PIN) == STATE_ON) {
    root["power"] = true;
  } else {
    root["power"] = false;
  }

  String res;
  root.printTo(res);

  webSocket.broadcastTXT(res);
}

void togglePower(bool value) {
  if (value) {
    digitalWrite(POWER_PIN, STATE_ON);
    if (currentSpeed == 0) {
      adjustSpeed(25);
    }
    Serial.println("Setting power to ON");
  } else {
    digitalWrite(POWER_PIN, STATE_OFF);
    setSpeedSetting("off");
    Serial.println("Setting power to OFF");
  }

  sendUpdate();
}


void setSpeedSetting(String setting) {
  if (setting != currentState) {
    currentState = setting;

    // turn all speed settings off everything off
    digitalWrite(SPEED_LOW_PIN, STATE_OFF);
    digitalWrite(SPEED_MED_PIN, STATE_OFF);
    digitalWrite(SPEED_MAX_PIN, STATE_OFF);

    // tiny delay between relay change over
    delay(15);

    if (setting == "low") {
      digitalWrite(SPEED_LOW_PIN, STATE_ON);
    } else if (setting == "med") {
      digitalWrite(SPEED_MED_PIN, STATE_ON);
    } else if (setting == "max") {
      digitalWrite(SPEED_MAX_PIN, STATE_ON);
    }
  }

  sendUpdate();
}

void adjustSpeed(int value) {
  if (value < 1) {
    // off
    togglePower(false);
    Serial.printf("Speed set to %d. Turning off\n", value);
  } else if (value < 34) {
    // set low
    setSpeedSetting("low");
    Serial.printf("Speed set to %d. Setting to low\n", value);
  } else if (value < 67) {
    // set medium
    setSpeedSetting("med");
    Serial.printf("Speed set to %d. Setting to medium\n", value);
  } else if (value < 101) {
    // set high
    setSpeedSetting("max");
    Serial.printf("Speed set to %d. Setting to high\n", value);
  } else {
    // invalid value - turn off
    togglePower(false);
    Serial.printf("Speed set to %d. Invalid Speed. Turning off.\n", value);
  }
}

void saveConfigCallback() {
  Serial.println("Resetting device...");
  delay(5000);
  resetRequired = true;
}

void setup(void) {
  // turn BUILT IN LED on at boot
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(POWER_PIN, OUTPUT);
  pinMode(SPEED_LOW_PIN, OUTPUT);
  pinMode(SPEED_MED_PIN, OUTPUT);
  pinMode(SPEED_MAX_PIN, OUTPUT);

  digitalWrite(POWER_PIN, STATE_OFF);
  digitalWrite(SPEED_LOW_PIN, STATE_OFF);
  digitalWrite(SPEED_MED_PIN, STATE_OFF);
  digitalWrite(SPEED_MAX_PIN, STATE_OFF);

  delay(1000);

  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  WiFi.mode(WIFI_STA);

  delay(1000);

  Serial.println("Starting...");

  // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // setup hostname
  String id = WiFi.macAddress();
  id.replace(":", "");
  id.toLowerCase();
  id = id.substring(6,12);
  id = "esp826-fan-" + id;
  id.toCharArray(hostname, 18);

  WiFi.hostname(hostname);
  Serial.println(hostname);

  // reset the device after config is saved
  wm.setSaveConfigCallback(saveConfigCallback);

  // sets timeout until configuration portal gets turned off
  wm.setTimeout(600);

  // first parameter is name of access point, second is the password
  if (!wm.autoConnect(hostname, "password")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);

    // reset and try again
    ESP.reset();
    delay(5000);
  }

  WiFi.hostname(hostname);

  // reset if flagged
  if (resetRequired) {
    ESP.reset();
  }

  // Add service to MDNS-sd
  delay(2000);

  if (MDNS.begin(hostname, WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  MDNS.addService("oznu-platform", "tcp", 81);
  MDNS.addServiceTxt("oznu-platform", "tcp", "type", "fan");
  MDNS.addServiceTxt("oznu-platform", "tcp", "mac", WiFi.macAddress());

  // start web socket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("Web socket server started on port 81");

  // start http update server
  httpUpdater.setup(&httpServer, update_path, update_username, update_password);

  httpServer.on("/", []() {
    if (!httpServer.authenticate(update_username, update_password)) {
      return httpServer.requestAuthentication();
    }
    String s = MAIN_page;
    httpServer.send(200, "text/html", s);
  });

  httpServer.begin();

  // turn LED off once ready
  digitalWrite(LED_BUILTIN, HIGH);    
}

void loop(void) {
  httpServer.handleClient();
  webSocket.loop();
  MDNS.update();
}
