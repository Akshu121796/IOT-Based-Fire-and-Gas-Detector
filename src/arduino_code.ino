#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "secrets.h"   // contains phoneNumber — copy secrets.h.example to secrets.h and fill it in

// --- PIN DEFINITIONS ---
// Sensors
const int FIRE_SENSOR_PIN = 7;   // DO from Flame Sensor
const int MQ5_ANALOG_PIN = A0;   // AO from MQ-5 Gas Sensor

// Outputs
const int BUZZER_PIN = 8;
const int RED_LED_PIN = 12;
const int YELLOW_LED_PIN = 11;

// GSM Module (TX->D2 RX->D3)
const int GSM_RX_PIN = 2;
const int GSM_TX_PIN = 3;
SoftwareSerial gsmSerial(GSM_RX_PIN, GSM_TX_PIN);

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- THRESHOLDS ---
const int GAS_THRESHOLD = 200;

bool hazardDetected = false;
bool smsSent = false;
bool gsmReady = false;

// --- BAUD RATES TO TRY FOR GSM ---
long baudRates[] = {9600, 19200, 38400, 57600, 115200};
int foundBaudRate = 0;

void setup() {
  Serial.begin(9600); // Serial to ESP32
  Serial.println("Arduino Booting...");

  pinMode(FIRE_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("GasFire System");
  delay(1000);

  lcd.setCursor(0, 1);
  lcd.print("Init GSM...");

  if (autoBaudDetect()) {
    gsmReady = true;
    lcd.setCursor(0, 1);
    lcd.print("GSM Ready ");
  } else {
    gsmReady = false;
    lcd.setCursor(0, 1);
    lcd.print("GSM FAIL! ");
  }
  delay(1000);
}

void loop() {
  int fireReading = digitalRead(FIRE_SENSOR_PIN);
  int gasReading = analogRead(MQ5_ANALOG_PIN);

  // Determine hazard
  if (fireReading == LOW || gasReading > GAS_THRESHOLD) {
    hazardDetected = true;
  } else {
    hazardDetected = false;
  }

  if (hazardDetected) {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);

    String alertMessage;
    lcd.setCursor(0, 0);
    lcd.print("!!! HAZARD !!! ");
    lcd.setCursor(0, 1);

    if (fireReading == LOW) {
      lcd.print("Fire Detected! ");
      alertMessage = "URGENT: FIRE detected!";
    } else {
      lcd.print("Gas Leak Alert! ");
      alertMessage = "Gas Leak! Value: " + String(gasReading);
    }

    if (!smsSent && gsmReady) {
      sendSMS(phoneNumber, alertMessage);
      smsSent = true;
    }
  } else {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);

    lcd.setCursor(0, 0);
    lcd.print("System: NORMAL ");
    lcd.setCursor(0, 1);
    lcd.print("Gas:");
    lcd.print(gasReading);
    lcd.print(" ");

    smsSent = false;
  }

  // Send data to ESP32
  Serial.print(gasReading);
  Serial.print(",");
  Serial.print(fireReading);
  Serial.print(",");
  Serial.println(hazardDetected ? "1" : "0");

  delay(1000);
}

// --- GSM Functions ---
bool autoBaudDetect() {
  for (int i = 0; i < sizeof(baudRates) / sizeof(long); i++) {
    gsmSerial.begin(baudRates[i]);
    delay(1000);
    for (int attempts = 0; attempts < 3; attempts++) {
      gsmSerial.println("AT");
      delay(200);
      if (gsmSerial.find("OK")) {
        foundBaudRate = baudRates[i];
        return true;
      }
    }
  }
  return false;
}

void sendSMS(const char* number, String message) {
  gsmSerial.println("AT+CMGF=1");
  delay(500);
  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(number);
  gsmSerial.println("\"");
  delay(500);
  gsmSerial.print(message);
  gsmSerial.write(26); // Ctrl+Z
  delay(5000);

  lcd.setCursor(0, 1);
  lcd.print("SMS SENT! ");
}
