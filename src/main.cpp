/*
Public thingsboard dashboard url: http://149.129.237.179:8080/dashboard/ebe92dd0-9f8b-11eb-ae50-993512ed21d9?publicId=4c47f6e0-9f8a-11eb-ae50-993512ed21d9

*/

#ifdef ESP32
#pragma message(THIS PROGRAM IS FOR ESP8266 ONLY !)
#error Select ESP8266 board.
#endif

#include <ArduinoOTA.h>
#include <Ticker.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
// #include <ESPAsync_WiFiManager.h> //https://github.com/khoih-prog/ESPAsync_WiFiManager
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <DHTesp.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <ThingsBoard.h>
#include "NTPTimer.h"
#include "wifi_id.h"

#define PIN_DHT11 0
#define PIN_SW 16
#define I2C_ADDRESS_TSL2561 0x39 // Light Sensor, TSL2561_ADDR_FLOAT

#define THINGSBOARD_TOKEN "KnqYBvkQQ0QloEMn9WF8"
#define THINGSBOARD_SERVER "149.129.237.179"
#define THINGSBOARD_SEND_INTERVAL 5

// AsyncWebServer webServer(80);
// DNSServer dnsServer;

WiFiUDP udp;
NTPTimer ntp(udp);
WiFiClient espClient;
ThingsBoard tb(espClient);
Ticker timerLedBlinkOff;

DHTesp dht;
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(I2C_ADDRESS_TSL2561, 12345);

char g_szChipID[9];

void OnTimer1Sec();
bool TSL2561_Init();
bool TSL2561_Read(float &lux);

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_SW, INPUT);
  delay(200);

  Serial.println("Booting...");
  dht.setup(PIN_DHT11, DHTesp::DHT11);
  TSL2561_Init();
  WiFiManager wifiManager;
  // wifiManager.setBreakAfterConfig(true);
  if (digitalRead(PIN_SW)==0)
  {
    Serial.println("WifiManager reset setting");
    wifiManager.resetSettings();
  }
  // wifiManager.autoConnect("RoomSensorAP");
  if (!wifiManager.autoConnect("RoomSensorAP", "binus123")) {
    Serial.println("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  
  // WiFi.mode(WIFI_STA);
  // WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();
  ntp.attach(OnTimer1Sec);
  ntp.begin();
  sprintf(g_szChipID, "%08X", ESP.getChipId());
  Serial.printf("ESP ChipID: %08X, hostname: %s\n", ESP.getChipId(), HOSTNAME);
  
}

void loop()
{
  ArduinoOTA.handle();
  yield();
}

void OnTimer1Sec()
{
  static int nCount = 0;
  if (!tb.connected()) {
  // Connect to the ThingsBoard
    Serial.print("Reconnecting to thingsboard server...");
    if (tb.connect(THINGSBOARD_SERVER, THINGSBOARD_TOKEN))
      Serial.println("connected.");
    else  
      Serial.println("Failed!");
  }
  // Serial.print("Hello World "); Serial.println(ntp.getFormattedDateTime());
  if (nCount % THINGSBOARD_SEND_INTERVAL == 0)
  {
    float lux = 0;
    TSL2561_Read(lux);
    float humidity = dht.getHumidity();
    float temperature = dht.getTemperature();
    if (dht.getStatus() == DHTesp::ERROR_NONE && tb.connected())
    {
      Serial.printf("Light: %.2f, Temp: %.2f, Hum: %.2f\n", lux, temperature, humidity);

      Serial.println("Sending data to thingsboard...");
      tb.sendTelemetryFloat("Temperature", temperature);
      tb.sendTelemetryFloat("Humidity", humidity);
      tb.sendTelemetryFloat("Light", lux);
    }
  }
  digitalWrite(LED_BUILTIN, LOW);
  timerLedBlinkOff.once_ms_scheduled(100, []() {
    digitalWrite(LED_BUILTIN, HIGH);
  });

  if (dht.getStatus() == DHTesp::ERROR_NONE)
    nCount++;
  if (nCount == 60)
    nCount = 0;
}

bool TSL2561_Init()
{
  bool fResult = tsl.begin();
  if (fResult)
  {
    sensor_t sensor;
    tsl.getSensor(&sensor);
    Serial.println("------------------------------------");
    Serial.print("Sensor:       ");
    Serial.println(sensor.name);
    Serial.print("Driver Ver:   ");
    Serial.println(sensor.version);
    Serial.print("Unique ID:    ");
    Serial.println(sensor.sensor_id);
    Serial.print("Max Value:    ");
    Serial.print(sensor.max_value);
    Serial.println(" lux");
    Serial.print("Min Value:    ");
    Serial.print(sensor.min_value);
    Serial.println(" lux");
    Serial.print("Resolution:   ");
    Serial.print(sensor.resolution);
    Serial.println(" lux");
    Serial.println("------------------------------------");

    // configure sensor
    /* You can also manually set the gain or enable auto-gain support */
    // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
    // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
    tsl.enableAutoRange(true); /* Auto-gain ... switches automatically between 1x and 16x */

    /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
    // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS); /* medium resolution and speed   */
    // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */
  }
  else
  {
    /* There was a problem detecting the TSL2561 ... check your connections */
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
  }
  return fResult;
}

bool TSL2561_Read(float &lux)
{
  bool fResult = true;
  sensors_event_t event;
  tsl.getEvent(&event);

  /* Display the results (light is measured in lux) */
  if (event.light)
  {
    uint16_t broadband = 0;
    uint16_t infrared = 0;

    /* Populate broadband and infrared with the latest values */
    tsl.getLuminosity(&broadband, &infrared);
    // Serial.printf("Light: %.2f lux, broadband: %d, IR: %d\n", event.light,
    //   broadband, infrared);
    lux = event.light;
  }
  else
  {
    /* If event.light = 0 lux the sensor is probably saturated
      and no reliable data could be generated! */
    Serial.println("Sensor overload");
    fResult = false;
  }
  return fResult;
}