#include <Arduino.h>
#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "time.h"
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

bool buttonstart = false;

Adafruit_ADS1115 ads;


const char* ssid = "mikesnet";
const char* password = "springchicken";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -14400;  //Replace with your GMT offset (secs)
const int daylightOffset_sec = 0;   //Replace with your daylight offset (secs)
int hours, mins, secs;

float tempSHT, humSHT;
int16_t adc0, adc1, adc2, adc3;
float volts0, volts1, volts2, volts3;
float tempC;

char auth[] = "NC-rTx2W1w-IPRycGolnSNHYW3BHdBKM";

AsyncWebServer server(80);

WidgetTerminal terminal(V10);

#define every(interval) \
    static uint32_t __every__##interval = millis(); \
    if (millis() - __every__##interval >= interval && (__every__##interval = millis()))

BLYNK_WRITE(V10) {
  if (String("help") == param.asStr()) {
    terminal.println("==List of available commands:==");
    terminal.println("wifi");
    terminal.println("reset");
    terminal.println("==End of list.==");
  }
  if (String("wifi") == param.asStr()) {
    terminal.print("Connected to: ");
    terminal.println(ssid);
    terminal.print("IP address:");
    terminal.println(WiFi.localIP());
    terminal.print("Signal strength: ");
    terminal.println(WiFi.RSSI());
    printLocalTime();
  }
  if (String("reset") == param.asStr()) {
    terminal.println("Restarting...");
    terminal.flush();
    ESP.restart();
  }
}

BLYNK_WRITE(V11)
{
  if (param.asInt() == 1) {buttonstart = true;}
  if (param.asInt() == 0) {buttonstart = false;}
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V11);
}

void printLocalTime() {
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  terminal.print(asctime(timeinfo));
}

void setup(void) {
  setCpuFrequencyMhz(80);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Blynk.config(auth, IPAddress(192, 168, 50, 197), 8080);
  Blynk.connect();
  while (!Blynk.connected()){}

  sensors.begin();
  sensors.requestTemperatures(); 
  tempC = sensors.getTempCByIndex(0);

  if (buttonstart) {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "Hi! I am ESP32.");
    });

    AsyncElegantOTA.begin(&server);    // Start ElegantOTA
    server.begin();
    Serial.println("HTTP server started");
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);


  struct tm timeinfo;
  getLocalTime(&timeinfo);
  hours = timeinfo.tm_hour;
  mins = timeinfo.tm_min;
  secs = timeinfo.tm_sec;
  ads.setGain(GAIN_ONE);  // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  ads.begin();
  adc0 = ads.readADC_SingleEnded(0);
  volts0 = ads.computeVolts(adc0)*2.0;
  Blynk.virtualWrite(V1, volts0);
  Blynk.virtualWrite(V2, tempC);
  Blynk.virtualWrite(V3, WiFi.RSSI());
  if (buttonstart){
    terminal.println("***Keith v1.1 STARTED***");
    terminal.print("Connected to ");
    terminal.println(ssid);
    terminal.print("IP address: ");
    terminal.println(WiFi.localIP());
    printLocalTime();
    terminal.print("Battery: ");
    terminal.print(volts0,3);
    terminal.print("v, Temp: ");
    terminal.println(tempC,3);
    terminal.flush();
  }
  Blynk.run();

  if (!buttonstart){
    esp_sleep_enable_timer_wakeup(55000000); // 60 sec
    esp_deep_sleep_start(); 
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {Blynk.run();}

  every(10000){
    sensors.requestTemperatures(); 
    tempC = sensors.getTempCByIndex(0);
    adc0 = ads.readADC_SingleEnded(0);
    volts0 = ads.computeVolts(adc0)*2.0;
    Blynk.virtualWrite(V1, volts0);
    Blynk.virtualWrite(V2, tempC);
    Blynk.virtualWrite(V3, WiFi.RSSI());
  }

}
