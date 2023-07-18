#include "freertos/event_groups.h"
// Users/amberhewett/Documents/Arduino/libraries/FreeRTOS/src/event_groups.c
#include <TFT_eSPI.h> // drawing on LED screen 
#include "SPI.h" // communication with LED screen 
#include <WiFi.h>
#include "time.h"
#include <HTTPClient.h> // Connect to internet for weather
//#include <OneButton.h> // Button library
//#define TOPBUTTON 35
#define BOTTOMBUTTON 34

#define BIT_0 (1<<0)

static EventGroupHandle_t xEventGroup;
int screenNum = 0; // which screen to present
//hw_timer_t * minuteTimer = NULL;

TFT_eSPI tft = TFT_eSPI();

// struct Button{
//   const uint8_t PIN;
//   bool pressed;
// };

// Button bottomButton = {BOTTOMBUTTON, false};

void IRAM_ATTR isr_changeScreen(){
  screenNum = (screenNum+1) % 3; // 3 possible screen choices
  BaseType_t xHigherPriorityTaskWoken, xResult;
  xHigherPriorityTaskWoken = pdFALSE; //
  xResult = xEventGroupSetBitsFromISR(
    xEventGroup,                // event group
    BIT_0,                      // bits to set
    &xHigherPriorityTaskWoken); // false 
  if(xResult != pdFAIL){
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void printScreen(void *arg){
  EventBits_t bit;
  bit = xEventGroupWaitBits(
    xEventGroup,             // event group
    BIT_0,                   // bit to wait for
    pdTRUE,                  // clears bit we are waiting for when TRUE
    pdFALSE,                 // waits for all bits when TRUE (AND), or FALSE (OR) 
    10000/portTICK_RATE_MS); // maximum wait 10 seconds

  if((bit & BIT_0) != 0){ // if bit was set
    switch(screenNum){
      case 0:
        screen0(); // will need task notification here (295)
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
}

const char* ssid = "twitch.tv/quin69";
const char* password = "Onigoroshi69";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * -4;
const int   daylightOffset_sec = 3600 * 0;
//openweatherkey = 
//location = 

void setup() {
Serial.begin(9600);
  delay(1000);
  Serial.println("baud rate 9600");

  pinMode(BOTTOMBUTTON, INPUT);
  attachInterrupt(BOTTOMBUTTON, isr_changeScreen, FALLING);
  xEventGroup = xEventGroupCreate();
  TaskHandle_t screen0Handle = NULL;
  TaskHandle_t screen1Handle = NULL;
  TaskHandle_t screen2Handle = NULL;

  xTaskCreate(
    screen0,    // name of function                        
    "screen0",  // descriptive name for task (for debugging purposes)
    10000,      // stack size in words 
    NULL,       // parameter
    3,          // priority
    &screen0Handle);      // passes handle to task being created
  xTaskCreate(screen1, "screen1", 10000, NULL, 3, &screen1Handle);
  xTaskCreate(screen2, "screen2", 10000, NULL, 3, &screen2Handle);
  xTaskCreate(printScreen, "printScreen", 10000, NULL, 4, NULL);


  // uint8_t minuteTimerID = 0;
  // uint16_t prescaler = 80;
  // timer = timerBegin(0, 80, true);
  // timerAttachInterrupt(timer, getTime, true);
  // timerAlarmWrite(timer, 60000000, true);
  // timerAlarmEnable(timer);

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

  tft.init();
  screen0();
}

void screen0(void *pvParameters){ // time, date, weather, location
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
  vTaskDelete(screen0Handle);
}

void screen1(void *pvParameters){ // screen 1 chronometer
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_WHITE, TFT_RED); // font, background
  tft.drawString("CHRONOMETER", 5, 120, 2);
  tft.drawString("START", 5, 140, 2);
  vTaskDelete(screen1Handle);
}

void screen2(void *pvParameters){ // screen 2 connects to health band
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE); // font, background
  tft.drawString("Heart Rate", 5, 120, 2);
  tft.drawString("Pulse", 5, 140, 2);
  vTaskDelete(screen2Handle);

}

void loop() {
  // put your main code here, to run repeatedly:

}
