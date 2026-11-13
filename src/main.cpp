#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoOTA.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_VEML7700.h>
#include <Adafruit_SHT4x.h>
#include <DallasTemperature.h>
#include <driver/rtc_io.h>
#include <esp_sntp.h>
#include "secrets.h" // copy secrets.example.h to secrets.h and fill it in

// Pins, as wired in docs/wiring.md.
#define SDA_PIN 19
#define SCL_PIN 20
#define SENSOR_PWR 18
#define ONE_WIRE 14
#define BUTTON 7
#define BATTERY 0
constexpr uint8_t MOISTURE[] = {2, 3, 4}; // one per pot. Pots 4-6 go on 5, 1, 6
constexpr int POTS = sizeof(MOISTURE);

// DS18B20 address of each pot's probe, in pot order. The debug session prints the addresses over
// serial. While these are zeros, probes are read in bus order, which won't match the pots.
DeviceAddress PROBES[POTS] = {};

const uint32_t READ_EVERY_MS = 10 * 60 * 1000;
const int UPLOAD_EVERY = 6;              // readings, so hourly
const uint32_t DEBUG_MS = 5 * 60 * 1000; // screen + Wi-Fi flashing window after a button press
const float BATTERY_DIVIDER = 2.0;       // onboard divider. Tune it until the screen matches a meter

const int16_t NONE = INT16_MIN; // sensor didn't answer
struct Reading {
  uint32_t time; // unix seconds, from a clock that's synced over NTP on every upload
  int16_t airC10, humidity10, soilC10[POTS]; // tenths
  uint16_t moistureMv[POTS], batteryMv;      // moisture is raw: lower is wetter, calibrate downstream
  float lux;
};

// RTC memory survives deep sleep.
const int MAX_READINGS = 144; // a day without Wi-Fi, then the oldest are dropped
RTC_DATA_ATTR Reading readings[MAX_READINGS];
RTC_DATA_ATTR int count = 0;
RTC_DATA_ATTR uint32_t wakes = 0;
RTC_DATA_ATTR int32_t channel = 0; // last access point, so reconnecting skips the scan
RTC_DATA_ATTR uint8_t bssid[6];

Adafruit_SSD1306 display(128, 64, &Wire, -1);
Adafruit_SHT4x sht4;
Adafruit_VEML7700 veml;
OneWire oneWire(ONE_WIRE);
DallasTemperature probes(&oneWire);

int16_t tenths(float v) { return isnan(v) ? NONE : lroundf(v * 10); }

Reading takeReading() {
  Reading r;
  r.time = time(nullptr);

  digitalWrite(SENSOR_PWR, HIGH);
  delay(100); // moisture sensors settle
  for (int i = 0; i < POTS; i++) {
    uint32_t mv = 0;
    for (int n = 0; n < 16; n++) mv += analogReadMilliVolts(MOISTURE[i]);
    r.moistureMv[i] = mv / 16;
  }
  probes.begin();
  probes.setResolution(10); // 0.25 °C, 190 ms per conversion instead of 750
  probes.requestTemperatures();
  for (int i = 0; i < POTS; i++) {
    // 0x28 is the DS18B20 family code, so a zero first byte means no address configured.
    float c = PROBES[i][0] ? probes.getTempC(PROBES[i]) : probes.getTempCByIndex(i);
    r.soilC10[i] = c == DEVICE_DISCONNECTED_C || c == 85 ? NONE : tenths(c); // 85 = power-on value
  }
  digitalWrite(SENSOR_PWR, LOW);

  sensors_event_t humidity, temp;
  bool sht = sht4.begin() && sht4.getEvent(&humidity, &temp);
  r.airC10 = sht ? tenths(temp.temperature) : NONE;
  r.humidity10 = sht ? tenths(humidity.relative_humidity) : NONE;
  r.lux = veml.begin() ? veml.readLux(VEML_LUX_AUTO) : NAN;
  veml.enable(false); // 45 µA down to 0.5 µA until the next begin()

  r.batteryMv = analogReadMilliVolts(BATTERY) * BATTERY_DIVIDER;
  return r;
}

void store(const Reading &r) {
  if (count == MAX_READINGS) memmove(readings, readings + 1, sizeof(Reading) * --count);
  readings[count++] = r;
}

bool connect() {
  WiFi.mode(WIFI_STA);
  if (channel) WiFi.begin(WIFI_SSID, WIFI_PASS, channel, bssid);
  else WiFi.begin(WIFI_SSID, WIFI_PASS);
  for (int i = 0; i < 100 && WiFi.status() != WL_CONNECTED; i++) delay(100); // up to 10 s
  if (WiFi.status() != WL_CONNECTED) {
    channel = 0; // the access point may have moved, scan next time
    return false;
  }
  channel = WiFi.channel();
  memcpy(bssid, WiFi.BSSID(), 6);
  configTime(0, 0, "pool.ntp.org"); // UTC, syncs in the background
  return true;
}

void waitForClock() {
  for (int i = 0; i < 30 && sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED; i++) delay(100);
}

String json(int16_t v) { return v == NONE ? "null" : String(v / 10.0, 1); }

bool upload() {
  if (!*UPLOAD_URL) return false;
  String body = "[";
  for (int i = 0; i < count; i++) {
    const Reading &r = readings[i];
    body += String(i ? "," : "") + "{\"time\":" + r.time + ",\"air_c\":" + json(r.airC10) +
            ",\"humidity\":" + json(r.humidity10) +
            ",\"lux\":" + (isnan(r.lux) ? "null" : String(r.lux, 1)) +
            ",\"battery_mv\":" + r.batteryMv + ",\"pots\":[";
    for (int p = 0; p < POTS; p++)
      body += String(p ? "," : "") + "{\"moisture_mv\":" + r.moistureMv[p] +
              ",\"soil_c\":" + json(r.soilC10[p]) + "}";
    body += "]}";
  }
  body += "]";
  HTTPClient http;
  http.begin(UPLOAD_URL);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  http.end();
  Serial.printf("Upload of %d readings: HTTP %d\n", count, code);
  return code >= 200 && code < 300;
}

void show(const Reading &r, const String &net, uint32_t left) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.printf("%s %lu:%02lu\n", net.c_str(), left / 60, left % 60);
  display.printf("Air %sC %s%%\n", json(r.airC10).c_str(), json(r.humidity10).c_str());
  display.printf("%.0f lx  %.2f V\n", r.lux, r.batteryMv / 1000.0);
  // ponytail: 3 header lines + 1 per pot, so a 6th pot falls off the bottom of the 8-line screen.
  for (int i = 0; i < POTS; i++)
    display.printf("P%d %4u mV %s C\n", i + 1, r.moistureMv[i], json(r.soilC10[i]).c_str());
  display.display();
}

// Power-on or button: screen on, Wi-Fi flashing enabled, for DEBUG_MS.
void debugSession() {
  bool screen = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (screen) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Connecting...");
    display.display();
  }

  digitalWrite(SENSOR_PWR, HIGH);
  probes.begin(); // needs power to find the probes
  DeviceAddress addr;
  for (int i = 0; probes.getAddress(addr, i); i++) {
    Serial.printf("Probe %d: {", i + 1);
    for (int b = 0; b < 8; b++) Serial.printf("0x%02X%s", addr[b], b < 7 ? ", " : "}\n");
  }
  digitalWrite(SENSOR_PWR, LOW);

  String net = connect() ? WiFi.localIP().toString() : "No Wi-Fi";
  Serial.println(net);
  ArduinoOTA.setHostname("greenhouse"); // reachable as greenhouse.local
  ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.begin();

  for (uint32_t last = 0; millis() < DEBUG_MS;) {
    ArduinoOTA.handle();
    if (last && millis() - last < 5000) continue;
    last = millis();
    Reading r = takeReading();
    Serial.printf("Air %s C %s %%, %.0f lx, battery %u mV\n", json(r.airC10).c_str(),
                  json(r.humidity10).c_str(), r.lux, r.batteryMv);
    for (int i = 0; i < POTS; i++)
      Serial.printf("  Pot %d: %u mV, %s C\n", i + 1, r.moistureMv[i], json(r.soilC10[i]).c_str());
    if (screen) show(r, net, (DEBUG_MS - millis()) / 1000);
  }
  if (screen) display.ssd1306_command(SSD1306_DISPLAYOFF); // ~10 µA while asleep
}

void sleep() {
  // The button pulls GPIO7 low. Keep its pull-up on through deep sleep.
  rtc_gpio_pullup_en((gpio_num_t)BUTTON);
  rtc_gpio_pulldown_dis((gpio_num_t)BUTTON);
  esp_sleep_enable_ext1_wakeup(1ULL << BUTTON, ESP_EXT1_WAKEUP_ANY_LOW);
  // Subtract the time spent awake so readings stay 10 min apart.
  uint32_t ms = millis() + 1000 < READ_EVERY_MS ? READ_EVERY_MS - millis() : 1000;
  esp_sleep_enable_timer_wakeup(ms * 1000ULL);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PWR, OUTPUT);
  Wire.begin(SDA_PIN, SCL_PIN);

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
    store(takeReading());
    if (++wakes % UPLOAD_EVERY == 0 && connect()) {
      if (upload()) count = 0;
      waitForClock(); // ponytail: the sleep clock drifts, so NTP resyncs it hourly
    }
  } else {
    debugSession();
  }
  sleep();
}

void loop() {}
