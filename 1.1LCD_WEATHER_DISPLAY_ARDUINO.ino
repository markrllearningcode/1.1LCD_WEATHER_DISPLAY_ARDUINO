#include "arduino_secrets.h"
#include <WiFiS3.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

LiquidCrystal_I2C lcd(0x27, 16, 2);

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;
char server[] = "api.openweathermap.org";

unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 60L * 1000L;

WiFiClient client;

void setup() {
  Serial.begin(9600);
  while(!Serial) {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");
  }

  if(WiFi.status() == WL_NO_MODULE) {
    Serial.println("Commmunication with WiFi module failed!");
    while(true)
    ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  while(status != WL_CONNECTED) {
    Serial.print("Attempting to conect to SSID: ");
    Serial.print(ssid);
    status = WiFi.begin(ssid, pass);
    delay(5000);
  }

  printWifiStatus();

  lcd.clear();
  lcd.print("WiFi connected");

  timeClient.begin();
  timeClient.setTimeOffset(3600 * 1);
}

void loop() {

  read_response();
  timeClient.update();

  if (!lastConnectionTime || millis() - lastConnectionTime > postingInterval) {
    httpRequest();
  }
}

void read_response() {
  String payload = "";
  bool jsonDetected = false;

  while(client.available()) {
    char c = client.read();

    Serial.print(c);

    if ('{' == c) {
      jsonDetected = true;
    }  
    if (jsonDetected) {
      payload += c;
    }
    received_data_num++;
  }
  if (jsonDetected) {
    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, payload);
    if (error) {
      Serial.print("Deserialization failed with code: ");
      Serial.println(error.c_str());
      return;
    }

    String weather = (root["weather"][0]["main"]);
    float temp = (float)(root["main"]["temp"]) -273.15;
    int humidity = root["main"]["humidity"];
    float pressure = (float)(root["main"]["pressure"]) / 1000;
    float wind_speed = root["wind"]["speed"];
    int wind_degree = root["wind"]["deg"];

    displayWeatherData(weather, temp, humidity, pressure, wind_speed);
  }
}

void httpRequest() {
  client.stop();

  String httpRequest = "";
  httpRequest += "GET /data/2.5/weather?q=" LOCATION "&appid=" API_KEY " HTTP/1.1";

  if (client.connect(server, 80)) {
    Serial.println("connected");
    client.println(httpRequest);
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();

    lastConnectionTime = millis();
  } else {
    Serial.println("connection failed");
  }
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println("dBm");
}

void displayWeatherData(String weather, float temp, int humidity, float pressure, float wind_speed) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  String daysOfTheWeek[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
  display.print(daysOfTheWeek[timeClient.getDay()]);

  display.print(" ");
  if (timeClient.getHours() < 10) display.print("0");
  display.print(timeClient.getHours());
  display.print(":");
  if (timeClient.getMinutes() < 10) display.print("0");
  display.println(timeClient.getMinutes());

  display.println();
  display.print(LOCATION);
  display.println(" " + weather);

  display.print("Temperature: ");
  display.print(temp);
  display.println(" C");

  display.print("Humidity: ");
  display.print(humidity);
  display.println(" %");

  display.print("Pressure: ");
  display.print(pressure);
  display.println(" bar");
  
  display.print("Wind: ");
  display.print(wind_speed);
  display.println(" m/s");

  display.display();
}
