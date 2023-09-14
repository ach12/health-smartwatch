# Smartwatch using ESP32
This project uses an ESP32 to make a smartwatch that determines the users location, their timezone and local weather conditions through API calls. Information is displayed on an LED screen. 

# About
The smartwatch displays typical information such as time, date, location and weather. The goal is to have the watch display the measurements other wearable devices which monitor users health. ![](https://github.com/ach12/Smartwatch-using-ESP32/blob/main/smartwatch-demo.gif)

# Schematic
![alt text](https://github.com/ach12/Smartwatch-using-ESP32/blob/main/ESP32-smartwatch-schematic.jpg?raw=true)

# Getting Started
LED screen specifications : green tab ST7735 128 x 160
  #define ST7735_GREENTAB2
  #define TFT_MOSI 23
  #define TFT_SCLK 18
  #define TFT_CS   5  // Chip select control pin
  #define TFT_DC   21  // Data Command control pin
  #define TFT_RST  -1  // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST
