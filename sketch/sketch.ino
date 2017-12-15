#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WebSocketsServer.h>
#include <ESP8266mDNS.h>

MDNSResponder mdns;
WebSocketsServer webSocket = WebSocketsServer(81);

// Replace with your network credentials
const char* ssid = "****";
const char* password = "****";

// Hostname
const char* accessoryName = "homebridge-fan";

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
  } else {
    digitalWrite(POWER_PIN, STATE_OFF);
    setSpeedSetting("off");
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


void setup(void) {
  delay(1000);

  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);

  WiFi.mode(WIFI_STA);
  WiFi.hostname(accessoryName);

  WiFi.begin(ssid, password);

  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(POWER_PIN, OUTPUT);
  pinMode(SPEED_LOW_PIN, OUTPUT);
  pinMode(SPEED_MED_PIN, OUTPUT);
  pinMode(SPEED_MAX_PIN, OUTPUT);

  digitalWrite(POWER_PIN, STATE_OFF);
  digitalWrite(SPEED_LOW_PIN, STATE_OFF);
  digitalWrite(SPEED_MED_PIN, STATE_OFF);
  digitalWrite(SPEED_MAX_PIN, STATE_OFF);

  if (mdns.begin(accessoryName, WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("Web socket server started on port 81");
}

void loop(void) {
  webSocket.loop();
}
