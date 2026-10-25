#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include "secrets.h" // copy secrets.example.h to secrets.h and fill it in

// Witty Cloud RGB LED, used as a status light since the top board has no serial.
#define LED_RED 15
#define LED_GREEN 12

Adafruit_SSD1306 display(128, 64, &Wire, -1);

bool found(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  Wire.begin(4, 5); // SDA = GPIO4, SCL = GPIO5

  // Most SSD1306 modules are 0x3C, some 0x3D.
  uint8_t addr = found(0x3C) ? 0x3C : found(0x3D) ? 0x3D : 0;
  bool hasDisplay = addr && display.begin(SSD1306_SWITCHCAPVCC, addr);
  if (hasDisplay) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("Greenhouse");
    display.setTextSize(1);
    display.println("\nConnecting...");
    display.display();
  }

  // Wi-Fi and OTA come before the display halt so a display fault can still be fixed over the air.
  Serial.printf("\nConnecting to %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) delay(500); // up to 15s
  String net = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "No Wi-Fi";
  Serial.printf("%s (status %d)\n", net.c_str(), WiFi.status());
  ArduinoOTA.setHostname("greenhouse"); // reachable as greenhouse.local
  ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.begin();

  if (!hasDisplay) {
    Serial.println("SSD1306 not found - check wiring");
    digitalWrite(LED_RED, HIGH);
    for (;;) { ArduinoOTA.handle(); delay(10); }
  }
  digitalWrite(LED_GREEN, HIGH);

  display.fillRect(0, 16, 128, 32, SSD1306_BLACK);
  display.setCursor(0, 24);
  display.println(net);
  display.display();
}

void loop() {
  ArduinoOTA.handle();

  // Uptime counter, proves the board is alive and not frozen.
  static unsigned long last = 0;
  if (millis() - last < 1000) return;
  last = millis();
  display.fillRect(0, 48, 128, 16, SSD1306_BLACK);
  display.setCursor(0, 50);
  display.printf("Up: %lus", millis() / 1000);
  display.display();
}
