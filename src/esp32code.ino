#include <WiFi.h>
#include "ThingSpeak.h"
#include "secrets.h"   // contains ssid, password, channelID, writeAPI — copy secrets.h.example to secrets.h and fill it in

WiFiClient client;

// --- Serial Communication with Arduino ---
#define RXD2 16 // ESP32 RX2 connects to Arduino TX (Pin 1)
#define TXD2 17 // optional

String serialData = "";
int gasValue = 0;
int fireValue = 0;
int hazardFlag = 0;

void setup() {
  Serial.begin(115200); // for debug
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); // from Arduino

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client);
}

void loop() {
  if (Serial2.available()) {
    serialData = Serial2.readStringUntil('\n');
    serialData.trim();

    int firstComma = serialData.indexOf(',');
    int secondComma = serialData.indexOf(',', firstComma + 1);

    if (firstComma > 0 && secondComma > firstComma) {
      gasValue = serialData.substring(0, firstComma).toInt();
      fireValue = serialData.substring(firstComma + 1, secondComma).toInt();
      hazardFlag = serialData.substring(secondComma + 1).toInt();

      Serial.print("Gas: "); Serial.print(gasValue);
      Serial.print(" | Flame: "); Serial.print(fireValue);
      Serial.print(" | Hazard: "); Serial.println(hazardFlag);

      ThingSpeak.setField(1, gasValue);
      ThingSpeak.setField(2, fireValue);
      ThingSpeak.setField(3, hazardFlag);

      int httpCode = ThingSpeak.writeFields(channelID, writeAPI);
      if (httpCode == 200) {
        Serial.println("ThingSpeak Update OK");
      } else {
        Serial.print("Error Code: ");
        Serial.println(httpCode);
      }

      delay(20000); // 20 seconds between updates
    }
  }
}
