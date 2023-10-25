#include "weathericons.h" // holds all the c arrays of the weather icons

#include <Arduino.h>
#include <TFT_eSPI.h> // drawing on LED screen. Library found on platformio registry
#include "SPI.h" // communication with LED screen 

// Connect to internet for weather
//#include <HTTPClient.h>
#include <ArduinoJson.h> // planchon
#include <WiFiClientSecure.h>
#include "time.h"

//#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_timer.h" // esp-idf has its own way to create software timers

#include <Adafruit_ADXL345_U.h>

#define BOTTOMBUTTON 34

static TimerHandle_t oneShotToZeroTimer = NULL;
static TimerHandle_t minuteAutoReloadTimer = NULL;
static TimerHandle_t accelerometerTimer = NULL;
int screenNum = 0; // which screen to present

TFT_eSPI tft = TFT_eSPI();

const char* ssid = "";
const char* password = "";


const char* ntpServer = "pool.ntp.org";
long  rawOffset = 0;
int   dstOffset = 0;

String googleMapsKey = "";
String weatherbitKey = "";

struct Weatherbit{
  String location; // Vancouver, CA
  String temp; // 27/18C
  int iconCode; // depending on weather code, display one of the bitmaps
} info;

#define BIT_0 (1<<0)
#define mainAUTO_RELOAD_TIMER_PERIOD pdMS_TO_TICKS(60000)
// static EventGroupHandle_t xEventGroup;
TaskHandle_t printScreenHandle = NULL;
TaskHandle_t screen0Handle = NULL;
TaskHandle_t screen1Handle = NULL;
TaskHandle_t screen2Handle = NULL;
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(0x53); // i2c address is 0x53...

//*****************************************************************************
// Callbacks
void printAccelerometerData(TimerHandle_t xTimer){
  Serial.println("printAccelerometerData");
  sensors_event_t event; 
  accel.getEvent(&event);
    
  /* Display the results (acceleration is measured in m/s^2) */
  Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print("  ");Serial.println("m/s^2 ");
}

void writeTimeCallback(TimerHandle_t xTimer){
  Serial.println("timerCallback");
  if(screenNum==0){
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    char time[50];
    strftime(time, sizeof(time), "%H:%M", &timeinfo);
    tft.drawString(time, 0, 5, 6); // text, xpos, ypos, size
    if(timeinfo.tm_hour == 0 && timeinfo.tm_min == 0){ // Still need to verify
      char date[50];
      strftime(date, sizeof(date), "%a %B %e", &timeinfo);
      tft.drawString(date, 5, 50, 2);
    }
    if((uint32_t)pvTimerGetTimerID(xTimer) == 0){
      xTimerStart(minuteAutoReloadTimer,0);
    }
  }
  else{
    xTimerStop(xTimer, 0);
  }
}

String geolocate(){ // gets the longitude and latitude based on the MAC address of the wifi

  WiFiClientSecure client;
  StaticJsonDocument<75> doc; //https://arduinojson.org/v6/assistant
  String jsonString = "";
  String latlng = "";

  Serial.println("\nStarting connection to server...");
  client.setInsecure();
  if (!client.connect("www.googleapis.com", 443))
    Serial.println("Connection failed!");
  else {
    Serial.println("GEOLOCATE - Connected to server!");
    client.println("POST https://www.googleapis.com/geolocation/v1/geolocate?key=" + googleMapsKey + " HTTP/1.0"); // Make a HTTP request
    client.println("Host: www.googleapis.com");
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.println("Content-Length: 0");
    client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }

    while (client.available()) {
      char c = client.read();
      Serial.write(c);
      jsonString += c;
    }

    client.stop();
  }

    char* jsonChar = &jsonString[0];

    DeserializationError error = deserializeJson(doc, jsonChar);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
    }
    else{
      latlng  = doc["location"]["lat"].as<String>(); // 45.4197248,-71.8667776
      latlng  += ","; // 45.4197248,-71.8667776
      latlng  += doc["location"]["lng"].as<String>(); // 45.4197248,-71.8667776

      Serial.println("latlng=" + latlng);
    }
  Serial.println("returning " + latlng);
  return latlng;
}

void timezone(String latlng){
  Serial.println("TIMEZONE. latlng = " + latlng);
  if (latlng == ""){
    Serial.println("failed to obtain latlng in geolocate()");
  }
  else{
    WiFiClientSecure client;
    time_t now;
    struct tm timeinfo;
    StaticJsonDocument<100> doc; // https://arduinojson.org/v6/assistant
    String jsonString = "";

    configTime(rawOffset, dstOffset, ntpServer); // raw and dst initially have values of zero
    getLocalTime(&timeinfo);
    time(&now);
    Serial.println("\nStarting connection to server...");
    client.setInsecure();
    if (!client.connect("maps.googleapis.com", 443))
      Serial.println("Connection failed!");
    else {
      Serial.println("TIME ZONE - Connected to server!");
      // Make a HTTP request:
      client.println("POST https://maps.googleapis.com/maps/api/timezone/json?location=" + latlng + "&timestamp=" + time(&now) + "&key=" + googleMapsKey + " HTTP/1.0");
      client.println("Host: maps.googleapis.com");
      client.println("Connection: close");
      client.println("Content-Type: application/json");
      client.println("Content-Length: 0");
      client.println();
      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          Serial.println("headers received");
          break;
        }
      }

      while (client.available()) {
        char c = client.read();
        Serial.write(c);
        jsonString += c;
      }

      client.stop();
    }
    // it would be rice to be able to filter json returned so that we do not have timeZoneId and timeZoneName
    // looks like it must be done on the client side? But then why do they say that certain responses are optional...
    char* jsonChar = &jsonString[0];

    DeserializationError error = deserializeJson(doc, jsonChar);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
    }
    else{
      Serial.print("deserializing...");
      rawOffset = doc["rawOffset"];
      dstOffset = doc["dstOffset"];
      configTime(rawOffset, dstOffset, ntpServer);
    }
  }
}

Weatherbit weather(String latlng){
  int delimiter = latlng.indexOf(",");
  String lat = latlng.substring(0, delimiter);
  String lng = latlng.substring(delimiter+1);

  Weatherbit info;
  WiFiClientSecure client;
  DynamicJsonDocument doc(1024); //https://arduinojson.org/v6/assistant
  String jsonString = "";
  String locationAndTemp = ""; 

  Serial.println("\nStarting connection to server...");
  client.setInsecure();
  if (!client.connect("api.weatherbit.io", 443))
    Serial.println("Connection failed!");
  else {
    Serial.println("WEATHER - Connected to server!");
    client.println("GET https://api.weatherbit.io/v2.0/forecast/daily?lat=" + lat + "&lon="+ lng + "&days=1&key=" + weatherbitKey + " HTTP/1.0");
    client.println("Host: api.weatherbit.io");
    client.println("Connection: close");
    client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }

    while (client.available()) {
      char c = client.read();
      //Serial.write(c);
      jsonString += c;
    }

    client.stop();
  }

  char* jsonChar = &jsonString[0];

  DeserializationError error = deserializeJson(doc, jsonChar);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
  }
  else{
    int maxx  = doc["data"][0]["max_temp"].as<int>(); // as int so we dont get any decima
    int minn  = doc["data"][0]["min_temp"].as<int>(); 
    info.temp = String(maxx) + "/" + String(minn);
    info.location = doc["city_name"].as<String>() + ", " + doc["country_code"].as<String>();
    info.iconCode = doc["data"][0]["weather"]["code"];
    Serial.println("info temp=" + info.temp);
    Serial.println("info location=" + info.location);
    Serial.print("info code=");
    Serial.print(info.iconCode);
  }
  return info;
}

void weatherIcon(int code){
  bool isDay = false;
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  if(timeinfo.tm_hour >= 6 && timeinfo.tm_hour < 18){ // Still need to verify
    isDay = true;
  }

  switch(code){
    case 200:
    case 201:
    case 202:
      tft.pushImage(0, 115, 21, 21, thunderrain); //xpos,ypos, xsize, ysize, name
    case 230:
    case 231:
    case 232:
    case 233:
      tft.pushImage(0, 115, 21, 21, thunder); //xpos,ypos, xsize, ysize, name
    case 300:
    case 301:
    case 302:
    case 511:
    case 520:
      tft.pushImage(0, 115, 21, 21,  drizzle);
    case 500:
    case 501:
    case 521:
      tft.pushImage(0, 115, 21, 21,  rain);
    case 502:
    case 522:
      tft.pushImage(0, 115, 21, 21,  heavyrain);
    case 600:
    case 601:
    case 623:
      tft.pushImage(0, 115, 21, 21,  snow);
    case 602:
    case 621:
    case 622:
      tft.pushImage(0, 115, 21, 21,  heavysnow);
    case 610:
    case 611:
    case 612:
      tft.pushImage(0, 115, 21, 21,  sleet);
    case 700:
    case 711:
    case 721:
    case 731:
    case 741:
      tft.pushImage(0, 115, 21, 21,  fog);
    case 803:
    case 804:
    case 900:
      tft.pushImage(0, 115, 21, 21,  clouds);
    case 800:
      if(isDay == true){ tft.pushImage(0, 115, 21, 21,  clearday);}
      else {tft.pushImage(0, 115, 21, 21,  clearnight);}
    case 801:
    case 802:
      if(isDay == true){ tft.pushImage(0, 115, 21, 21,  partlycloudyday);}
      else {tft.pushImage(0, 115, 21, 21,  partlycloudynight);}
  }
}
//*****************************************************************************
// accelerometer

void displaySensorDetails(void)
{
  sensor_t sensor;
  accel.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" m/s^2");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" m/s^2");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" m/s^2");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

void displayDataRate(void)
{
  Serial.print  ("Data Rate:    "); 
  
  switch(accel.getDataRate())
  {
    case ADXL345_DATARATE_3200_HZ:
      Serial.print  ("3200 "); 
      break;
    case ADXL345_DATARATE_1600_HZ:
      Serial.print  ("1600 "); 
      break;
    case ADXL345_DATARATE_800_HZ:
      Serial.print  ("800 "); 
      break;
    case ADXL345_DATARATE_400_HZ:
      Serial.print  ("400 "); 
      break;
    case ADXL345_DATARATE_200_HZ:
      Serial.print  ("200 "); 
      break;
    case ADXL345_DATARATE_100_HZ:
      Serial.print  ("100 "); 
      break;
    case ADXL345_DATARATE_50_HZ:
      Serial.print  ("50 "); 
      break;
    case ADXL345_DATARATE_25_HZ:
      Serial.print  ("25 "); 
      break;
    case ADXL345_DATARATE_12_5_HZ:
      Serial.print  ("12.5 "); 
      break;
    case ADXL345_DATARATE_6_25HZ:
      Serial.print  ("6.25 "); 
      break;
    case ADXL345_DATARATE_3_13_HZ:
      Serial.print  ("3.13 "); 
      break;
    case ADXL345_DATARATE_1_56_HZ:
      Serial.print  ("1.56 "); 
      break;
    case ADXL345_DATARATE_0_78_HZ:
      Serial.print  ("0.78 "); 
      break;
    case ADXL345_DATARATE_0_39_HZ:
      Serial.print  ("0.39 "); 
      break;
    case ADXL345_DATARATE_0_20_HZ:
      Serial.print  ("0.20 "); 
      break;
    case ADXL345_DATARATE_0_10_HZ:
      Serial.print  ("0.10 "); 
      break;
    default:
      Serial.print  ("???? "); 
      break;
  }  
  Serial.println(" Hz");  
}

void displayRange(void)
{
  Serial.print  ("Range:         +/- "); 
  
  switch(accel.getRange())
  {
    case ADXL345_RANGE_16_G:
      Serial.print  ("16 "); 
      break;
    case ADXL345_RANGE_8_G:
      Serial.print  ("8 "); 
      break;
    case ADXL345_RANGE_4_G:
      Serial.print  ("4 "); 
      break;
    case ADXL345_RANGE_2_G:
      Serial.print  ("2 "); 
      break;
    default:
      Serial.print  ("?? "); 
      break;
  }  
  Serial.println(" g");  
}


// ISR
void IRAM_ATTR isr_changeScreen(){ // no Serial.print in an ISR
  BaseType_t xHigherPriorityTaskWoken;
  // BaseType_t xResult;
  xHigherPriorityTaskWoken = pdFALSE;
  // xResult = xEventGroupSetBitsFromISR(
  //   xEventGroup,                // event group
  //   BIT_0,                      // bits to set
  //   &xHigherPriorityTaskWoken); // false 
  // //vTaskNotifyGiveFromISR(printScreenHandle, xHigherPriorityTaskWoken);
  // if(xResult != pdFAIL){
  //   portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  // }
  vTaskNotifyGiveFromISR(printScreenHandle, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
//*****************************************************************************
// Tasks
void printScreen(void *pvParameters){
  for(;;){
    // EventBits_t bit;
    // bit = xEventGroupWaitBits(
    //   xEventGroup,             // event group
    //   BIT_0,                   // bit to wait for
    //   pdTRUE,                  // clears bit we are waiting for when TRUE
    //   pdFALSE,                 // waits for all bits when TRUE (AND), or FALSE (OR) 
    //   10000/portTICK_RATE_MS); // maximum wait 10 seconds
    uint32_t ulEventsToProcess;

    ulEventsToProcess = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if(ulEventsToProcess != 0){
      screenNum = (screenNum+1) % 3; // 3 possible screen choices
      Serial.print("printScreen, screenNum = ");
      Serial.print(screenNum);
    // if((bit & BIT_0) != 0){ // if bit was set
      switch(screenNum){
        case 0:
          xTaskNotifyGive(screen0Handle);
          break;
        case 1:
          xTaskNotifyGive(screen1Handle);
          break;
        case 2:
          xTaskNotifyGive(screen2Handle);
          break;
        default:
          Serial.println("screenNum out of range");
      }
    }
  }
}

void screen0(void *pvParameters){ // time, date, weather, location
  for(;;){
    uint32_t ulEventsToProcess;
    ulEventsToProcess = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // nust set INCLUDE_vTaskSuspend to 1 in FreeRTOSConfig.h
    if(ulEventsToProcess != 0){
      Serial.println(" screen0");

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
      tft.drawString(info.location, 5, 140, 2); // size 2 font is 16 pixel length
      tft.drawString(info.temp+"°C", 25, 120, 2);
      tft.setSwapBytes(true);
      weatherIcon(info.iconCode);

      xTimerChangePeriod(oneShotToZeroTimer, pdMS_TO_TICKS((60-timeinfo.tm_sec)*1000), 0);
    }
  }
}

void screen1(void *pvParameters){ // screen 1 chronometer
  for(;;){
    uint32_t ulEventsToProcess;
    ulEventsToProcess = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // nust set INCLUDE_vTaskSuspend to 1 in FreeRTOSConfig.h
    if(ulEventsToProcess != 0){
      Serial.println(" screen1");
      tft.fillScreen(TFT_RED);
      tft.setTextColor(TFT_WHITE, TFT_RED); // font, background
      tft.drawString("PLACEHOLDER", 5, 140, 2);
    }
  }
}

void screen2(void *pvParameters){ // displaying accelerometer data
  for(;;){
    uint32_t ulEventsToProcess;
    ulEventsToProcess = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // nust set INCLUDE_vTaskSuspend to 1 in FreeRTOSConfig.h
    if(ulEventsToProcess != 0){
      /* Get a new sensor event */ 
      xTimerStart(accelerometerTimer,0);
    }
  }
}

//*****************************************************************************
// Setup
void setup() {  

  tft.init();
  tft.fillScreen(TFT_BLACK);

  Serial.begin(9600);
  delay(1000);
  Serial.println("baud rate 9600");
  
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to network");
  // uint8_t status = WiFi.waitForConnectResult();
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {       
    Serial.println("Connected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
  } 

  // String latlng = geolocate();
  // timezone(latlng); // determines timezone based on coordinates

  // // String location = reverseGeocoding(latlng);
  // info = weather(latlng); // global Weatherbit struct info

  // accelerometer data
 if(!accel.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    while(1);
  }

  /* Set the range to whatever is appropriate for your project */
  accel.setRange(ADXL345_RANGE_16_G);
  // accel.setRange(ADXL345_RANGE_8_G);
  // accel.setRange(ADXL345_RANGE_4_G);
  // accel.setRange(ADXL345_RANGE_2_G);
  
  /* Display some basic information on this sensor */
  displaySensorDetails();
  
  /* Display additional settings (outside the scope of sensor_t) */
  displayDataRate();
  displayRange();
  Serial.println("");


  pinMode(BOTTOMBUTTON, INPUT);
  attachInterrupt(BOTTOMBUTTON, isr_changeScreen, FALLING);


  xTaskCreate(
    screen0,    // name of function                        
    "screen0",  // descriptive name for task (for debugging purposes)
    10000,      // stack size in words 
    NULL,       // parameter
    4,          // priority
    &screen0Handle);      // passes handle to task being created
  xTaskCreate(screen1, "screen1", 10000, NULL, 4, &screen1Handle);
  xTaskCreate(screen2, "screen2", 10000, NULL, 4, &screen2Handle);
  xTaskCreate(printScreen, "printScreen", 10000, NULL, 3, &printScreenHandle);

  oneShotToZeroTimer = xTimerCreate("oneshot", mainAUTO_RELOAD_TIMER_PERIOD, pdFALSE, (void*)0, writeTimeCallback);

  minuteAutoReloadTimer = xTimerCreate("auto-reload timer", mainAUTO_RELOAD_TIMER_PERIOD, pdTRUE, (void*)1, writeTimeCallback);

  accelerometerTimer = xTimerCreate("accelerometer timer", mainAUTO_RELOAD_TIMER_PERIOD, pdTRUE, (void*)2, printAccelerometerData);

  xTaskNotifyGive(screen0Handle); 

}

void loop() {}
