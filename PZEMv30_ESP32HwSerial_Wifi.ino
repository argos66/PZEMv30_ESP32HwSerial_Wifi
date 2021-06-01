#include <HardwareSerial.h> // Serie
#include <PZEM004Tv30.h> //PZEM004T
#include <WiFi.h> // Wifi
#include <WebServer.h> //Web Server
#include <HTTPUpdateServer.h> //Web Update Server
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> //Affichage LCD 16x2
#include "time.h" //NTP

#define WAIT 2
#define ONBOARD_LED 2
#define LCD_COLUMNS 16
#define LCD_ROWS 2

const char* ssid     = ""; //Wifi SSID
const char* password = ""; //Wifi password
const char* ntpServer = "fr.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
uint32_t chipId = 0;

LiquidCrystal_I2C lcd(0x27,LCD_COLUMNS,LCD_ROWS);  // set the LCD address to 0x27 for a 16 chars and 2 line display
// SDA on D21 - SCL on D22

WebServer httpServer(80); //Listen on 80
HTTPUpdateServer httpUpdater;

HardwareSerial PzemSerial2(2); // Use hwserial UART2 at pins IO-16 (RX2) and IO-17 (TX2)
PZEM004Tv30 pzem(&PzemSerial2);

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) Serial.println("Failed to obtain time");
  Serial.println(&timeinfo, "NTP Time : %d-%m-%Y %H:%M:%S");
}

void handleRoot() {
  String httpMessage = "\n<html>\n<body>\n";  

  for(int i=0; i<17; i=i+8) { chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i; }

  httpMessage += "ESP32 Chip model = <b>" + String(ESP.getChipModel()) + "</b> (ID : " + String(chipId) + ") Rev " + String(ESP.getChipRevision()) + " (with " + String(ESP.getChipCores()) + " core)<br />\n";
  httpMessage += "PZEM004T output on : <a href=\"/output.json\">output.json</a><br />\n";
  httpMessage += "HTTPUpdateServer ready on <a href=\"/update\">/update</a><br />\n";
  httpMessage += "\n</body>\n</html>\n";
  httpServer.send(200, "text/html", httpMessage);
}

void handleJson() {
  float pzem_voltage = pzem.voltage();
  float pzem_current = pzem.current();
  float pzem_power = pzem.power();
  float pzem_energy = pzem.energy();
  float pzem_frequency = pzem.frequency();
  float pzem_pf = pzem.pf();
  struct tm timeinfo;
  String jsonString = "{\n";

  if (isnan(pzem_voltage)) pzem_voltage = -1;
  if (isnan(pzem_current)) pzem_current = -1;
  if (isnan(pzem_power)) pzem_power = -1;
  if (isnan(pzem_energy)) pzem_energy = -1;
  if (isnan(pzem_frequency)) pzem_frequency = -1;
  if (isnan(pzem_pf)) pzem_pf = -1;

  if(getLocalTime(&timeinfo)){
    char timeStringBuff[30];
    jsonString += "\"date\":\"";
    strftime(timeStringBuff, sizeof(timeStringBuff), "%d-%m-%Y %H:%M:%S", &timeinfo);
    jsonString += String(timeStringBuff) + "\",\n";
  }

  jsonString += "\"v\":" + String(pzem_voltage) + ",\n\"a\":" + String(pzem_current) + ",\n\"w\":" + String(pzem_power) + ",\n\"kwh\":" + String(pzem_energy) + ",\n\"hz\":" + String(pzem_frequency) + ",\n\"pf\":" + String(pzem_pf) + "\n}\n";
  httpServer.setContentLength(jsonString.length());
  httpServer.send(200, "application/json", jsonString);
}

void handleNotFound() {
  struct tm timeinfo;
  char timeStringBuff[30];

  if(getLocalTime(&timeinfo)) strftime(timeStringBuff, sizeof(timeStringBuff), "%d-%m-%Y %H:%M:%S", &timeinfo);
  String message = String(timeStringBuff) + " - File Not Found. ";
  message += "URI: " + httpServer.uri();
  httpServer.send(404, "text/html", message);
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
  WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  httpServer.on("/", handleRoot);
  httpServer.on("/output.json", handleJson);
  httpServer.onNotFound(handleNotFound);
  httpUpdater.setup(&httpServer);
  httpServer.begin();

  for(int i=0; i<17; i=i+8) { chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i; }
  
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  lcd.setCursor(0,0); // Ligne 1
  lcd.print("ESP32 I2C LCD !");
  lcd.setCursor(0,1); // Ligne 2
  lcd.print(String(ESP.getChipModel()));

  Serial.printf("ESP32 Chip model = %s Rev %d (with %d core)\n", ESP.getChipModel(), ESP.getChipRevision(), ESP.getChipCores());
  Serial.print("Chip ID: "); Serial.println(chipId);
  Serial.println("HTTP Server started");
}

void loop() {
  httpServer.handleClient();
  delay(WAIT); //allow the cpu to switch to other tasks
}
