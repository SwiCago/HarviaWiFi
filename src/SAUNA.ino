/*
  SAUNA.ino - Harvia sauna heater control code for Sonoff TH esp8266 based devices.
  Copyright (c) 2019 Al Betschart.  All right reserved.
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "SAUNA.h"
#include <FS.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <PubSubClient.h>
//Note PubSubClient.h has a MQTT_MAX_PACKET_SIZE of 128 defined, so either raise it to 256 or use short topics
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

// ===============================
//
// WARNING: Modifing your Sauna heater with WiFi can cause death if you are not careful. 
// You are dealing with 240V, so take precautions not to kill yourself. I am not responsible 
// if you kill yourself or burn your sauna/house down. You have been warned.
//
// ===============================

#define BUTTON_PIN 0    // Sonoff button
#define RELAY_PIN 12    // Sonoff relay
#define LED_PIN 13      // Sonoff blue LED
#define SENSOR_PIN 14   // Sonoff sensor DS

//DALLAS Sensors can exceed 80C, TH sensors cannot! So use DS
#define TEMPERATURE_PRECISION 9
OneWire oneWire(SENSOR_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress sensor;

// wifi/mqtt variables (these are set by config page and then stored in SPIFFS)
char ap_ssid[32];
char ap_pwd[64];
char host_name[10];
char mqtt_server[40];
char mqtt_port[6];
char mqtt_user[10];
char mqtt_pwd[10];
char client_id[10];
char sub_topic[40];
char set_topic[40];
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

// Device Variables
const unsigned long DEBOUNCE = 200;    // the debounce time, increase if the output flickers
const unsigned long REBOOT = 5000;     // reboot timout for button
const unsigned long FACTORY = 30000;   // factory reset, returns to captive protal mode
const unsigned int SEND_INTERVAL = 5000; // interval to return sauan status
boolean deviceOn = false;       // device On or Off
int buttonState = LOW;          // variable for reading the pushbutton status
int lastButtonState = HIGH;     // previous state of the button
int relayState = LOW;           // device actively powered
unsigned long lastTime = 0;     // the last time the output pin was toggled
unsigned long lastTempSend;     // the last time the temperature was read/sent
unsigned long onTime;           // the time the device went to ON status
float setTemp = 60.0;           // set temperatur, app adjustable
float offSetTemp = 0.5;         // does not need adjusting, this prevents short cycling. The heated rocks make sauna go beyond offset
boolean delayedOff = false;     // shall off timer start when temperature is reached? Config page adjustable, stored in SPIFFS
int maxRunTime = 60;            // max time the sauna will run, depends on abpve variable.  Config page adjustable, stored in SPIFFS
boolean firstTemp = false;      // first time temperature reached, for delayed off timer
float maxTemp = 90.0;           // max temp in Celcius. Config page adjustable, stored in SPIFFS

//Captive portal variables, only used for config page
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
IPAddress netMsk(255, 255, 255, 0);
DNSServer dnsServer;
ESP8266WebServer server(80);
boolean captive = false;

// ============================================================================
// SETUP
void setup() {
  Serial.begin(9600);
  Serial.println("Booting");
  init_IO();
  setDefaults();
  esp_info();
  load_config();
  if (init_wifi()) {
    init_DS();
    init_OTA();
    init_MQTT();
  }
  else {
    init_captivePortal();
  }
  Serial.println("Ready");
}

// Initial IO and force sauna into off status
void init_IO() {
  pinMode(BUTTON_PIN, INPUT);  // on/off button
  pinMode(RELAY_PIN, OUTPUT);  // relay
  digitalWrite(RELAY_PIN, relayState);  // relay off
  pinMode(LED_PIN, OUTPUT);    // led
  digitalWrite(LED_PIN, LOW);  // always on
}

// Set default values on boot, will be over written by SPIFFS read after first config save
void setDefaults() {
  strcpy(ap_ssid, "");
  strcpy(ap_pwd, "");
  strcpy(host_name, "sauna");
  strcpy(mqtt_server, "");
  strcpy(mqtt_port, "");
  strcpy(mqtt_user, "");
  strcpy(mqtt_pwd, "");
  strcpy(client_id, "");
  strcpy(sub_topic, "");
  strcpy(set_topic, "");
  delayedOff = false;
  maxRunTime = 60;
  maxTemp = 90.0;
}

// Initialize Dallas Sensor
void init_DS() {
  sensors.begin();//only for Dallas
  sensors.getAddress(sensor, 0);//only for Dallas
  sensors.setResolution(sensor, TEMPERATURE_PRECISION);
}

// Initialize WIFI and decide if Captive Portal AP mode or client mode
boolean init_wifi() {
  if (ap_ssid[0] == '\0') {
    Serial.println("\n\r \n\rStarting in AP mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP("Sauna");
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
    return false;
  }
  else {
    WiFi.hostname(host_name);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ap_ssid, ap_pwd);
    Serial.print("\n\r \n\rWorking to connect");
    //Wait for connection to wifi ap when in client mode
    while (WiFi.status() != WL_CONNECTED) {
      ledStatus(false, 250);
      ledStatus(true, 250);
      Serial.print(".");
      checkButton(); //emergency reset option, incase of bad password
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ap_ssid);
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  }
}

// Enable OTA only when connected as a client.
void init_OTA() {
  Serial.println("Start OTA Listener");
  ArduinoOTA.setHostname(host_name);
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    ledStatus((progress / (total / 100)) % 2 ? false : true, 0);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

// Init MQTT only when connected as a client.
void init_MQTT() {
  mqtt_client.setServer(mqtt_server, atoi(mqtt_port));
  mqtt_client.setCallback(mqttCallback);
  mqttConnect();
  Serial.println("MQTT connected");
}

// Subscribe to MQTT topic and set callbacks
void mqttConnect() {
  while (!mqtt_client.connected()) {   // Loop until we're reconnected
    if (mqtt_client.connect(client_id, mqtt_user, mqtt_pwd)) { // Attempt to connect
      mqtt_client.subscribe(set_topic);
    } else {
      delay(5000); // Wait 5 seconds before retrying
    }
  }
}

// MQTT callback, this is where we process received data from broker
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Copy payload into message buffer
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  if (strcmp(topic, set_topic) == 0) { //if the incoming message is on the boiler_set_topic topic...
    // Parse message into JSON
    const size_t bufferSize = JSON_OBJECT_SIZE(2);
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject& root = jsonBuffer.parseObject(message);
    if (!root.success()) {
      mqtt_client.publish(sub_topic, "!root.success(): invalid JSON on set_topic...");
      return;
    }
    if (root.containsKey("state")) {
      boolean state = root["state"];
      deviceOn = state;
      if (deviceOn) {
        onTime = millis();
        firstTemp = false;
      }
      ledStatus(false, 100);
      ledStatus(true, 100);
      ledStatus(false, 100);
      ledStatus(true, 0);
    }
    else if (root.containsKey("setpoint")) {
      setTemp = root["setpoint"] > maxTemp ? maxTemp : root["setpoint"];
    }
    else {
      mqtt_client.publish(sub_topic, "sauna: update failed");
    }
  }
}

// MQTT publish, this is where we data to broker
void sendData(float tempC, float tempF) {
  const size_t bufferSize = JSON_OBJECT_SIZE(6);
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject& root = jsonBuffer.createObject();
  root["celcius"] = tempC;
  root["fahrenheit"] = tempF;
  root["state"] = deviceOn == true ? 1 : 0;
  root["active"] = relayState == HIGH ? 1 : 0;
  root["setpoint"] = setTemp;
  int timeLeft = round(((onTime + (maxRunTime * 60 * 1000)) - millis()) / (60 * 1000)) + 1;
  root["autooff"] = !deviceOn || timeLeft < 0 || timeLeft > maxRunTime ? 0 : timeLeft;

  char buffer[512];
  root.printTo(buffer, sizeof(buffer));

  bool retain = true;
  if (!mqtt_client.publish(sub_topic, buffer, retain)) {
    mqtt_client.publish(sub_topic, "failed to publish to topic");
  }
  ledStatus(false, 200);
  ledStatus(true, 0);
}

// Initialize captive portal page
void init_captivePortal() {
  Serial.println("Starting captive portal");
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  server.on("/", handle_root);
  server.on("/generate_204", handle_root);
  server.on("/save", handle_save);
  server.onNotFound(handleNotFound);
  server.begin();
  captive = true;
}

// Handler for page not found, always return 200
void handleNotFound() {
  Serial.print("\t\t\t\t URI Not Found: ");
  Serial.println(server.uri());
  server.send ( 200, "text/plain", "URI Not Found" );
}

// Handler to serve our config page
void handle_root() {
  Serial.println("Root served");
  String toSend = setup_page;
  toSend.replace("AP_LIST", scan());
  server.send(200, "text/html", toSend);
  delay(100);
}

// Handler to serve or save page
void handle_save() {
  Serial.println("Save served");
  if (server.hasArg("submit")) {
    save_config(server.arg("ap_ssid"), server.arg("ap_pwd"), server.arg("host_name"),
                server.arg("mqtt_host"), server.arg("mqtt_port"), server.arg("mqtt_user"),
                server.arg("mqtt_pwd"), server.arg("mqtt_id"), server.arg("mqtt_sub"),
                server.arg("mqtt_pub"), server.arg("delayoff"), server.arg("maxtime"),
                server.arg("maxtemp"));
  }
  String toSend = save_page;
  server.send(200, "text/html", toSend);
  delay(100);
  ESP.restart();
}

// Scan for wifi AP availabilty, used for Config page to help select an AP
String scan() {
  String wifiList = "";
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    String linkText = ssid_link;
    // Print SSID and RSSI for each network found
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.print(")");
    Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
    linkText.replace("SSID_DETAIL", WiFi.SSID(i) + " ( " + WiFi.RSSI(i) + " )");
    linkText.replace("SSID", WiFi.SSID(i));
    delay(10);
    wifiList += linkText;
  }
  return wifiList;
}

// CHIP info, mainly for debugging
void esp_info() {
  uint32_t realSize = ESP.getFlashChipRealSize();
  uint32_t ideSize = ESP.getFlashChipSize();
  FlashMode_t ideMode = ESP.getFlashChipMode();

  Serial.printf("Chip ID: %08x", ESP.getChipId());
  Serial.printf("Flash real id:   %08X\n", ESP.getFlashChipId());
  Serial.printf("Flash real size: %u bytes\n\n", realSize);

  Serial.printf("Flash ide  size: %u bytes\n", ideSize);
  Serial.printf("Flash ide speed: %u Hz\n", ESP.getFlashChipSpeed());
  Serial.printf("Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));

  if (ideSize != realSize) {
    Serial.println("Flash Chip configuration wrong!\n");
  } else {
    Serial.println("Flash Chip configuration ok.\n");
  }
}

// Helper function for making LED blink
void ledStatus(boolean state, int delay_ms) {
  digitalWrite(LED_PIN, state ? LOW : HIGH);
  if (delay_ms > 0) {
    delay(delay_ms);
  }
}

// Read DS temperatur value and send to MQTT topic
void readSensor() {
  if (millis() < (lastTempSend + SEND_INTERVAL)) { // only send the temperature every Xms as defined at top
    return;
  }
  sensors.requestTemperatures();
  float c = sensors.getTempC(sensor);
  float f = DallasTemperature::toFahrenheit(c);
  sendData(c, f);
  changeRelayState(c);
  lastTempSend = millis();
}

// Turn on/off Sauna based on status and temperature
void changeRelayState(float temp) {
  if (deviceOn && delayedOff && !firstTemp) {
    onTime = millis();
  }
  if(deviceOn) {
    checkTime();
  }
  if (deviceOn && temp < setTemp - offSetTemp) {
    relayState = HIGH;
    digitalWrite(RELAY_PIN, relayState);
  }
  else if ((!deviceOn) || (deviceOn && temp > setTemp + offSetTemp)) {
    relayState = LOW;
    digitalWrite(RELAY_PIN, relayState);
    if (deviceOn && delayedOff && !firstTemp) {
      firstTemp = true;
    }
  }

}

// Auto off timer
void checkTime() {
  if (deviceOn && millis() > (onTime + (maxRunTime * 60 * 1000))) {
    deviceOn = false;
  }
}

void load_config() {
  delay(1000);
  if (SPIFFS.begin()) {
    //save_config("", "", "", "", "", "", "", "", "", "", "false", "60", "90.0"); //we can use this if we need to add more config, so SPIFF will match
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        const size_t bufferSize = JSON_OBJECT_SIZE(10);
        DynamicJsonBuffer jsonBuffer(bufferSize);
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(ap_ssid, json["ap_ssid"]);
          strcpy(ap_pwd, json["ap_pwd"]);
          strcpy(host_name, json["host_name"]);
          strcpy(mqtt_server, json["mqtt_host"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_pwd, json["mqtt_pwd"]);
          strcpy(client_id, json["mqtt_id"]);
          strcpy(sub_topic, json["mqtt_sub"]);
          strcpy(set_topic, json["mqtt_pub"]);
          delayedOff = json["delayedOff"];
          maxRunTime = json["maxRunTime"];
          maxTemp  = json["maxTemp"];
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
}

void save_config(String apSsid, String apPwd, String hostName,
                 String mqttHost, String mqttPort, String mqttUser,
                 String mqttPwd, String mqttId, String mqttSub,
                 String mqttPub, String delayoff, String maxtime,
                 String maxtemp) {
  const size_t bufferSize = JSON_OBJECT_SIZE(12);
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& json = jsonBuffer.createObject();
  json["ap_ssid"] = apSsid;
  json["ap_pwd"] = apPwd;
  json["host_name"] = hostName;
  json["mqtt_host"] = mqttHost;
  json["mqtt_port"] = mqttPort;
  json["mqtt_user"] = mqttUser;
  json["mqtt_pwd"] = mqttPwd;
  json["mqtt_id"] = mqttId;
  json["mqtt_sub"] = mqttSub;
  json["mqtt_pub"] = mqttPub;
  json["delayedOff"] = delayoff == "" ? "false" : delayoff;
  json["maxRunTime"] = maxtime  == "" ? "60" : maxtime;
  json["maxTemp"] = maxtemp == "" ? "90.0" : maxtemp;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  json.printTo(Serial);
  json.printTo(configFile);
  configFile.close();
}


void checkButton() {
  int buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW && lastButtonState == HIGH) {
    lastTime = millis();
  }
  else if (buttonState == HIGH && lastButtonState == LOW  && millis() - lastTime > FACTORY) {
    save_config("", "", "", "", "", "", "", "", "", "", "false", "60", "90.0");
    delay(100);
    ESP.restart();
  }
  else if (buttonState == HIGH && lastButtonState == LOW  && millis() - lastTime > REBOOT) {
    ESP.restart();
  }
  else if (buttonState == HIGH && lastButtonState == LOW && millis() - lastTime > DEBOUNCE) {
    deviceOn = deviceOn == true ? false : true;
    if (deviceOn) {
      onTime = millis();
      firstTemp = false;
    }
    ledStatus(false, 100);
    ledStatus(true, 100);
    ledStatus(false, 100);
    ledStatus(true, 0);
  }
  lastButtonState = buttonState;
}


// ===============================
// Program LOOP
void loop() {
  if (captive) {
    dnsServer.processNextRequest();
    server.handleClient();
  }
  else {
    ArduinoOTA.handle();
    if (!mqtt_client.connected()) {
      mqttConnect();
    }
    checkButton();
    readSensor();
    mqtt_client.loop();
  }
}
