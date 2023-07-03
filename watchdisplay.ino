// watchdisplay.ino
// writing to LED screen, want three different watch displays
// screen 0 : time, date, weather, location
// screen 1 : chronometer, minuter
// screen 2 : connetion to health band

// LED screen 
#include <TFT_eSPI.h>
#include "SPI.h"
// Wifi
#include <WiFi.h>
#include "time.h"
// Weather
#include <HTTPClient.h>

TFT_eSPI tft = TFT_eSPI();
//TFT_eSprite img = TFT_eSprite(&tft);

const char* ssid = "";
const char* password = "";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * -4;
const int   daylightOffset_sec = 3600 * 0;
//openweatherkey = 
//location = 


void setup(){
  Serial.begin(9600);
  delay(1000);
  Serial.println("setup complete");

  tft.init();
  Serial.begin(9600);
  Serial.println("initialized");
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to network");

  uint8_t status = WiFi.waitForConnectResult();
  if (status == WL_CONNECTED) {       
    Serial.println("Connected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
    
  } 

  // We configure the NTP server
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void screen0(){ // time, date, weather, location
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
  }

  char time[50];
  strftime(time, sizeof(time), "%H:%M", &timeinfo);
  char date[50];
  strftime(date, sizeof(date), "%a %B %e", &timeinfo);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // font, background
  tft.drawString(time, 0, 5, 6); // text, xpos, ypos, size
  tft.drawString(date, 5, 50, 2);
  tft.drawString("Sherbrooke, CA", 5, 120, 2);
  tft.drawString("24/9C", 5, 140, 2);
  delay(1000);
}
void screen1(){ // screen 1 chronometer
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_WHITE, TFT_RED); // font, background
  tft.drawString("CHRONOMETER", 5, 120, 2);
  tft.drawString("START", 5, 140, 2);
  delay(1000);
}

void screen2(){ // screen 2 connects to health band
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE); // font, background
  tft.drawString("Heart Rate", 5, 120, 2);
  tft.drawString("Pulse", 5, 140, 2);
  delay(1000);
}
void loop(){
  screen2();
}