#include <HardwareSerial.h> // Serie
#include <PZEM004Tv30.h> //PZEM004T
#include <WiFi.h> // Wifi
#include <WebServer.h> //Web Server
#include "time.h" //NTP

#define wait 2
#define ONBOARD_LED  2

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
  if(!getLocalTime(&timeinfo)) Serial.println("Failed to obtain time");
  Serial.println(&timeinfo, "NTP Time : %d-%M-%Y %H:%M:%S");
}

void handleRoot() {
  server.send(200, "text/html", "\n<html>\n<body>\nESP32 Dev module <br /> Get result on : <a href=\"/output.json\">output.json</a>\n</body>\n</html>\n");
}

void handleJson() {
  float pzem_voltage = pzem.voltage();
  float pzem_current = pzem.current();
  float pzem_power = pzem.power();
  float pzem_energy = pzem.energy();
  float pzem_frequency = pzem.frequency();
  float pzem_pf = pzem.pf();
  struct tm timeinfo;
  String jsonString = "{";

  if(getLocalTime(&timeinfo)){
    char timeStringBuff[30];
    jsonString += "\"date\":\"";
    strftime(timeStringBuff, sizeof(timeStringBuff), "%d-%M-%Y %H:%M:%S", &timeinfo);
    jsonString += String(timeStringBuff) + "\",";
  }

  jsonString += "\"v\":" + String(pzem_voltage) + ",\"a\":" + String(pzem_current) + ",\"w\":" + String(pzem_power) + ",\"kwh\":" + String(pzem_energy) + ",\"hz\":" + String(pzem_frequency) + ",\"pf\":" + String(pzem_pf) + "}\n";

  server.setContentLength(jsonString.length());
  server.send(200, "application/json", jsonString);
}

void handleNotFound() {
  struct tm timeinfo;
  char timeStringBuff[30];

  if(getLocalTime(&timeinfo)) strftime(timeStringBuff, sizeof(timeStringBuff), "%d-%M-%Y %H:%M:%S", &timeinfo);
  String message = String(timeStringBuff) + " - File Not Found. ";
  message += "URI: " + server.uri();
  server.send(404, "text/html", message);
  Serial.println(message);
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  digitalWrite(ONBOARD_LED,HIGH);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //init and get the time
  printLocalTime();
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  digitalWrite(ONBOARD_LED,LOW);
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.begin(ssid, password);
}

void setup() {
  pinMode(ONBOARD_LED,OUTPUT);
  digitalWrite(ONBOARD_LED,LOW);
  Serial.begin(115200);
  WiFi.disconnect(true); // delete old config
  delay(500);
  WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  server.on("/", handleRoot);
  server.on("/output.json", handleJson);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  delay(wait); //allow the cpu to switch to other tasks
}
