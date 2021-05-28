#include <HardwareSerial.h>
#include <PZEM004Tv30.h>
#include <WiFi.h>

#define wait 1000

const char* ssid     = ""; //Wifi SSID
const char* password = ""; //Wifi password

WiFiServer server(80); //Listen on 80
HardwareSerial PzemSerial2(2); // Use hwserial UART2 at pins IO-16 (RX2) and IO-17 (TX2)
PZEM004Tv30 pzem(&PzemSerial2);

void setup() {
    Serial.begin(115200);

/* Setup WiFi Connexion */
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("WiFi connected ! IP address: ");
    Serial.println(WiFi.localIP());

    server.begin();
/* End WiFi Setup */
}

void loop() {
    WiFiClient client = server.available();   // listen for incoming clients
    float pzem_voltage = pzem.voltage();
    float pzem_current = pzem.current();
    float pzem_power = pzem.power();
    float pzem_energy = pzem.energy();
    float pzem_frequency = pzem.frequency();
    float pzem_pf = pzem.pf();

    if (client) {                             // if you get a client,
      String jsonString = "{";                // make a String to hold incoming data from the client
      while (client.connected()) {            // loop while the client's connected
        if (client.available()) {             // if there's bytes to read from the client,
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println();

            jsonString += "\"v:\""; //Volts
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

            client.println(); // The HTTP response ends with another blank line:
            break; // break out of the while loop:
        }
      }
      jsonString += "}\n";
      client.println(jsonString); //print jsonString sur port http
      Serial.println(jsonString); //print jsonString sur port série
      client.stop(); // close the connection:
    }

    delay(wait);
}
