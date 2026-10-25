#include <Wire.h>
#include <Adafruit_SSD1306.h>

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
  if (!addr || !display.begin(SSD1306_SWITCHCAPVCC, addr)) {
    Serial.println("SSD1306 not found - check wiring");
    digitalWrite(LED_RED, HIGH);
    for (;;) delay(1000);
  }
  digitalWrite(LED_GREEN, HIGH);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("Greenhouse");
  display.setTextSize(1);
  display.println("\nHello from ESP8266!");
  display.display();
}

void loop() {
  // Uptime counter, proves the board is alive and not frozen.
  display.fillRect(0, 48, 128, 16, SSD1306_BLACK);
  display.setCursor(0, 50);
  display.printf("Up: %lus", millis() / 1000);
  display.display();
  delay(1000);
}
