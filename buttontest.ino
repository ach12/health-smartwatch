// buttontest.ino 
// Testing push buttons as interrupts
// Buttons have pull up resistor, so pressing them gives a LOW output

int topButton = 35; // top push button connected to digital pin 35
int bottomButton = 34; // buttom push button connected to digital pin 1
int screen = 0; // which screen to present

// may want to add software debouncing later if needed
//unsigned long buttonTime = 0;
//unsigned long lastButtonTime = 0;

void IRAM_ATTR printChrono(){
  Serial.println("chronometre");
}

void IRAM_ATTR printScreen(){
  screen = (screen+1) % 3; // 3 possible screen choices
  Serial.printf("screen %d\n", screen);
}

void setup(void){
  Serial.begin(9600);
  delay(1000);
  Serial.println("Hello");
  pinMode(topButton, INPUT);
  pinMode(bottomButton, INPUT);
  attachInterrupt(topButton, printChrono, FALLING);
  attachInterrupt(bottomButton, printScreen, FALLING);
  Serial.println("Initialized");
}

void loop(){
  Serial.println("...");
  delay(1000);
}


