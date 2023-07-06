// watchdisplay.ino
// writing to LED screen, want three different watch displays
// screen 0 : time, date, weather, location
// screen 1 : chronometer, minuter
// screen 2 : connetion to health band

// LED screen specifications : green tab ST7735 128 x 160
// setup for this screen can be found in Arduino/libraries/TFT_eSPI/User_Setup.h
  // #define ST7735_GREENTAB2
  // #define TFT_MOSI 23
  // #define TFT_SCLK 18
  // #define TFT_CS   5  // Chip select control pin
  // #define TFT_DC   21  // Data Command control pin
  // #define TFT_RST  -1  // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST

// /Users/amberhewett/Documents/Arduino/libraries/FreeRTOS/src/FreeRTOSConfig.h

#include <TFT_eSPI.h> // drawing on LED screen 
#include "SPI.h" // communication with LED screen 
#include <WiFi.h>
#include "time.h"
#include <HTTPClient.h> // Connect to internet for weather

#define TOPBUTTON 35
#define BOTTOMBUTTON 34

int screenNum = 0; // which screen to present
int state = 1;
int prevState = 1;

TFT_eSPI tft = TFT_eSPI();

const char* ssid = "twitch.tv/quin69";
const char* password = "Onigoroshi69";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * -4;
const int   daylightOffset_sec = 3600 * 0;
//openweatherkey = 
//location = 

// void IRAM_ATTR printChrono(){
//   Serial.println("chronometre");
// }

// void IRAM_ATTR printScreen(){
//   screenNum = (screenNum+1) % 3; // 3 possible screen choices
//   Serial.printf("screen %d\n", screenNum);
//   changeScreen = true;
// }

void setup(){
  Serial.begin(9600);
  delay(1000);
  Serial.println("baud rate 9600");

  pinMode(TOPBUTTON, INPUT);
  pinMode(BOTTOMBUTTON, INPUT);
  // attachInterrupt(TOPBUTTON, printChrono, FALLING);
  // attachInterrupt(BOTTOMBUTTON, printScreen, FALLING);

  Serial.println("initialized");
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to network");
  uint8_t status = WiFi.waitForConnectResult();
  if (status == WL_CONNECTED) {       
    Serial.println("Connected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);   // We configure the NTP server

  tft.init();
  screen0();
}

void loop() {  
  state = digitalRead(BOTTOMBUTTON); // 1 not pressed; 0 pressed
  if (state != prevState){
    delay(250);
    Serial.print("state:"); Serial.println(state);
    Serial.print("prevState:"); Serial.println(prevState);
    screenChange();
  }
}

void screenChange(){
  screenNum = (screenNum+1) % 3; // 3 possible screen choices
  switch(screenNum){
    case 0:
      screen0();
      break;
    case 1:
      screen1();
      break;
    case 2:
      screen2();
      break;
    default:
      Serial.println("screenNum out of range");
  }
}

void screen0(){
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
}

void screen1(){
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_WHITE, TFT_RED); // font, background
  tft.drawString("CHRONOMETER", 5, 120, 2);
  tft.drawString("START", 5, 140, 2);
}

void screen2(){
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE); // font, background
  tft.drawString("Heart Rate", 5, 120, 2);
  tft.drawString("Pulse", 5, 140, 2);
}