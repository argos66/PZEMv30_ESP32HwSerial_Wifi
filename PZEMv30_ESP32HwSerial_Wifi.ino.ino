#include <HardwareSerial.h>
#include <PZEM004Tv30.h>
#include <WiFi.h>
#include <WebServer.h>
#include "time.h"

#define wait 2

const char* ssid     = ""; //Wifi SSID
const char* password = ""; //Wifi password

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

WebServer server(80); //Listen on 80
HardwareSerial PzemSerial2(2); // Use hwserial UART2 at pins IO-16 (RX2) and IO-17 (TX2)
PZEM004Tv30 pzem(&PzemSerial2);

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "NTP Time : %m%d%Y - %H:%M:%S");
}

void handleRoot() {
  server.send(200, "text/plain", "ESP32 Dev module");
}

void handleJson() {
  float pzem_voltage = pzem.voltage();
  float pzem_current = pzem.current();
  float pzem_power = pzem.power();
  float pzem_energy = pzem.energy();
  float pzem_frequency = pzem.frequency();
  float pzem_pf = pzem.pf();
  String jsonString = "{";
  struct tm timeinfo;

  if(getLocalTime(&timeinfo)){
    char timeStringBuff[50];
    jsonString += "\"date:\"";
    strftime(timeStringBuff, sizeof(timeStringBuff), "%m%d%Y - %H:%M:%S", &timeinfo);
    jsonString += timeStringBuff;
  }

  //TODO : tester le sprintf
/*
  char values[100];
  sprintf(values, ",\"v:\"%s", pzem_voltage);
*/
  jsonString += ",\"v:\""; //Volts
  jsonString += pzem_voltage;
  jsonString += ",\"a:\""; //Ampere
  jsonString += pzem_current;
  jsonString += ",\"w:\""; //Watt
  jsonString += pzem_power;
  jsonString += ",\"kwh:\""; //kWh
  jsonString += pzem_energy;
  jsonString += ",\"hz:\""; //Hz
  jsonString += pzem_frequency;
  jsonString += ",\"pf:\""; //Power Factor
  jsonString += pzem_pf;
  jsonString += "}\n";

  server.setContentLength(jsonString.length());
  server.send(200, "application/json", jsonString);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  printLocalTime();
  Serial.println(message);
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Connected to AP successfully!");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //init and get the time
  printLocalTime();
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  printLocalTime();
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.begin(ssid, password);
}

void setup() {
  Serial.begin(115200);
  WiFi.disconnect(true); // delete old config
  delay(1000);
  WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  WiFi.begin(ssid, password);
  server.on("/", handleRoot);
  server.on("/output.json", handleJson);
  server.onNotFound(handleNotFound);
}

void loop() {
  server.handleClient();
  delay(wait); //allow the cpu to switch to other tasks
}
