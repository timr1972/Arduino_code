boolean varDebug = 1; // 1 to get basic serial output, 2 for Verbose

/****************** Loaded Modules ****************/
#include <Streaming.h>
#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>

/****************** Static Values ****************/
#define NEOPIN 12


/********************* Variable *******************/
char inData[10];
byte index = 0;
const int numberOfButtons = 6;
//                           0  1  2  3  4  5
byte buttons[]            = {2, 3, 4, 5, 6, 7}; // TL, Beam, Fog, Horn, Lights, TR
byte buttonState[]        = {1, 1, 1, 1, 1, 1};
byte oldButtonState[]     = {1, 1, 1, 1, 1, 1};
long lastButtonPush[]     = {0, 0, 0, 0, 0, 0};
byte specialButtonState   = 0;
long checklight           = 0;
long sendingKeepalive     = 0;
long sendingTimeout       = 0;
byte sending              = 0;
byte senddataFlag         = 0;
byte dashState[5];
byte leftState            = 0;
byte rightState           = 0;
byte lightState           = 0;
byte beamState            = 0;
byte fogState             = 0;
byte leds                 = 13;
int neoBrightness         = 80; // 80, 50, 10
int deBounceTimer         = 125;
int checklightDuration    = 50;
char byteIn[2];
int x                     = 0;
byte tempval;

/**************** Serial Ports ******************/
#define rxPin 10
#define txPin 9
SoftwareSerial BTSerial(rxPin, txPin); // RX, TX 
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, NEOPIN, NEO_GRB + NEO_KHZ800);


/********************* Easy Transfer *********************/
struct SEND_DATA_STRUCTURE{
  byte turnleftButton = 0;
  byte beamButton     = 0;
  byte fogButton      = 0;
  byte lightButton    = 0;
  byte turnrightButton= 0;
  byte hornButton     = 0;
  byte checkDigit     = 0;
};

SEND_DATA_STRUCTURE senddata;

struct OLD_DATA_STRUCTURE{
  byte turnleftButton  = senddata.turnleftButton;
  byte beamButton      = senddata.beamButton;
  byte fogButton       = senddata.fogButton;
  byte lightButton     = senddata.lightButton;
  byte turnrightButton = senddata.turnrightButton;
  byte hornButton      = senddata.hornButton;
};

OLD_DATA_STRUCTURE oldData;
/********************* Easy Transfer *********************/


/*********************************************/
/*                                           */
/*            Startup routine                */
/*                                           */
/*********************************************/
void setup() {
  Serial.begin(115200);         // Start serial 0 at 115.2k used for varDebugging
  BTSerial.begin(9600);         // Steering Wheel via Bluetooth
  
  strip.begin();
  for(int i=0; i < numberOfButtons; i++) {
    pinMode(buttons[i], INPUT_PULLUP);
  }
  pinMode(leds, OUTPUT); // 13
  delay(50);  
  for(x = 1; x < 17; x++) {
    strip.setPixelColor(x,strip.Color(0,0,255));
    strip.show();
    delay(30);
    strip.setPixelColor(x,strip.Color(0,0,0));
    strip.show();
    delay(30);
  }
  delay(250);
  BTSerial << "9,2\n";
  Serial.println("V7_Wheel is up");
}

/*********************************************/
/*                                           */
/*                Main Loop                  */
/*                                           */
/*********************************************/
void loop() {
  buttonCheck(); // This checks for unique button pushes
  /*** Check buttons ***/
  senddata.turnleftButton = !digitalRead(buttons[0]);
  senddata.beamButton     = !digitalRead(buttons[1]);
  senddata.fogButton      = !digitalRead(buttons[2]);
  senddata.hornButton     = !digitalRead(buttons[3]);
  senddata.lightButton    = !digitalRead(buttons[4]);
  senddata.turnrightButton= !digitalRead(buttons[5]);
  
  if(senddata.turnleftButton != oldButtonState[0] && millis()>lastButtonPush[0]){
    oldButtonState[0] = senddata.turnleftButton;
    lastButtonPush[0] = millis() + deBounceTimer;
    sendButton(0,senddata.turnleftButton);
  }

  if(senddata.beamButton != oldButtonState[1] && millis()>lastButtonPush[1]){
    oldButtonState[1] = senddata.beamButton;
    lastButtonPush[1] = millis() + deBounceTimer;
    sendButton(1,senddata.beamButton);
  }

  if(senddata.fogButton != oldButtonState[2] && millis()>lastButtonPush[2]){
    oldButtonState[2] = senddata.fogButton;
    lastButtonPush[2] = millis() + deBounceTimer;
    sendButton(2,senddata.fogButton);
  }

  if(senddata.hornButton != oldButtonState[3] && millis()>lastButtonPush[3]){
    oldButtonState[3] = senddata.hornButton;
    lastButtonPush[3] = millis() + deBounceTimer;
    sendButton(3,senddata.hornButton);
  }
  
  if(senddata.lightButton != oldButtonState[4] && millis()>lastButtonPush[4]){
    oldButtonState[4] = senddata.lightButton;
    lastButtonPush[4] = millis() + deBounceTimer;
    sendButton(4,senddata.lightButton);
  }

  if(senddata.turnrightButton != oldButtonState[5] && millis()>lastButtonPush[5]){
    oldButtonState[5] = senddata.turnrightButton;
    lastButtonPush[5] = millis() + deBounceTimer;
    sendButton(5,senddata.turnrightButton);
  }
  getBluetooth();
}
/******************* End of Main Loop ********************/

/********************************************
Check for unique button push combinations 
*********************************************/
void buttonCheck() {
  int specialButtons = 0;
  while (digitalRead(buttons[0])==LOW and digitalRead(buttons[1])==LOW and digitalRead(buttons[2])==LOW ) specialButtons = 1 ;
  while (digitalRead(buttons[0])==LOW and digitalRead(buttons[5])==LOW )                                  specialButtons = 2 ;
  while (digitalRead(buttons[1])==LOW and digitalRead(buttons[2])==LOW ) specialButtons = 3 ;
  switch (specialButtons) {
    case 1:
      if (varDebug) Serial << "Reset requested...\n\r\n\r";
      delay(100);
      asm volatile ("  jmp 0");
      break;
    case 2:
      if(specialButtonState == 2) {
        if (varDebug) Serial << "Stop Flashing Hazards...(Not Active)\n\r";
        specialButtonState = 0;
      } else {
        if (varDebug) Serial << "Flash Hazards...(Not Active)\n\r";
        specialButtonState = 2;
      }
      delay(100);
      break;
    case 3:
      if (varDebug) Serial << "Do Something special...(Not Active)\n\r";
      //specialButtonState = 3;
      BTSerial << "9,3\n";
      delay(50);
      break;
  }
}

/*********************************************
 Carry out actions based on received data
*********************************************/
void changeWheelState(int pos, int state){
  if (varDebug > 1) Serial << "Actions " << pos << " - " << state << "\n\r";
  switch (pos) {
    case 0: // Turn Left
      switch (state) {
        case 0: // Off
          strip.setPixelColor(1,strip.Color(0,0,0));
          strip.setPixelColor(2,strip.Color(0,0,0));
          strip.setPixelColor(3,strip.Color(0,0,0));
          strip.show();
          break;
        case 1: // Short
          strip.setPixelColor(1,strip.Color(0,155,0));
          strip.setPixelColor(3,strip.Color(0,155,0));
          strip.show();
          break;
        case 2: // Long
          strip.setPixelColor(1,strip.Color(0,155,0));
          strip.setPixelColor(2,strip.Color(0,155,0));
          strip.setPixelColor(3,strip.Color(0,155,0));
          strip.show();
          break;
      }
      break;
    case 1: // Main Beam
      switch (state) {
        case 0: // Beam Off 2-0
          strip.setPixelColor(4,strip.Color(0,0,0));
          strip.setPixelColor(5,strip.Color(0,0,0));
          strip.show();
          break;
        case 1: // Beam On 2-1
          strip.setPixelColor(4,strip.Color(0,0,255));
          strip.setPixelColor(5,strip.Color(0,0,255));
          strip.show();
          break;
      }
      break;
    case 2: // Fog
      switch (state) {
        case 0: // Fog Lights off
          strip.setPixelColor(11,strip.Color(0,0,0));
          strip.setPixelColor(12,strip.Color(0,0,0));
          strip.show();
          break;
        case 1: // Fog Lights off
          strip.setPixelColor(11,strip.Color(255,140,0));
          strip.setPixelColor(12,strip.Color(255,140,0));
          strip.show();
          break;
      }
      break;
    case 4: // Lights
      switch (state) {
        case 0: // Side Lights off 4-0
          neoBrightness = 80;
          strip.setBrightness(neoBrightness);
          strip.setPixelColor(6,strip.Color(  0, 0, 0));  // Left of centre -2 
          strip.setPixelColor(7,strip.Color(  0, 0, 0));  // Left of centre -1
          strip.setPixelColor(8,strip.Color(  0, 0, 0));  // Bottom Middle
          strip.setPixelColor(9,strip.Color(  0, 0, 0));  // Right of centre +1
          strip.setPixelColor(10,strip.Color( 0, 0, 0));  // Right of centre +2
          strip.show();
          digitalWrite(leds, LOW); // Button illumination off
          break;
        case 1: // Side Lights on 4-1
          neoBrightness = 50;
          strip.setBrightness(neoBrightness);
          strip.setPixelColor(6 ,strip.Color(  0, 55, 55));    // Left of centre -2 
          strip.setPixelColor(7 ,strip.Color(  0, 155, 155));  // Left of centre -1
          strip.setPixelColor(8 ,strip.Color(  0, 255, 255));  // Bottom Middle
          strip.setPixelColor(9 ,strip.Color(  0, 155, 155));  // Right of centre +1
          strip.setPixelColor(10,strip.Color(  0, 55, 55));    // Right of centre +2
          strip.show();
          digitalWrite(leds, HIGH); // Button illumination on
          break;
        case 2: // Head Lights on 4-2
          neoBrightness = 10;
          strip.setBrightness(neoBrightness);
          strip.setPixelColor(6 ,strip.Color(  0, 55, 55));    // Left of centre -2 
          strip.setPixelColor(7 ,strip.Color(  0, 155, 155));  // Left of centre -1
          strip.setPixelColor(8 ,strip.Color(  0, 255, 255));  // Bottom Middle
          strip.setPixelColor(9 ,strip.Color(  0, 155, 155));  // Right of centre +1
          strip.setPixelColor(10,strip.Color(  0, 55, 55));    // Right of centre +2
          strip.show();
          digitalWrite(leds, HIGH); // Button illumination on
          break;
        case 3: // Side Lights on 4-3
          neoBrightness = 50;
          strip.setBrightness(neoBrightness);
          strip.setPixelColor(6 ,strip.Color(  0, 55, 55));    // Left of centre -2 
          strip.setPixelColor(7 ,strip.Color(  0, 155, 155));  // Left of centre -1
          strip.setPixelColor(8 ,strip.Color(  0, 255, 255));  // Bottom Middle
          strip.setPixelColor(9 ,strip.Color(  0, 155, 155));  // Right of centre +1
          strip.setPixelColor(10,strip.Color(  0, 55, 55));    // Right of centre +2
          strip.show();
          digitalWrite(leds, HIGH); // Button illumination on
          break;
      }
      // Do Something
      break;
    case 5: // Turn Right
      switch (state) {
        case 0: // Off
          strip.setPixelColor(15,strip.Color(0,0,0));
          strip.setPixelColor(14,strip.Color(0,0,0));
          strip.setPixelColor(13,strip.Color(0,0,0));
          strip.show();
          break;
        case 1: // Short
          strip.setPixelColor(15,strip.Color(0,155,0));
          strip.setPixelColor(13,strip.Color(0,155,0));
          strip.show();
          break;
        case 2: // Long
          strip.setPixelColor(15,strip.Color(0,155,0));
          strip.setPixelColor(14,strip.Color(0,155,0));
          strip.setPixelColor(13,strip.Color(0,155,0));
          strip.show();
          break;
      }
      break;
    case 9: // Restart Wheel
      switch (state) {
        case 0:
          strip.setPixelColor(0,strip.Color(0,0,0));
          strip.show();
          break;
        case 1:
          strip.setPixelColor(0,strip.Color(155,155,155));
          strip.show();
          break;
        case 9:
          if (varDebug > 0) Serial << "Reset requested...\n\r\n\r";
          delay(100);
          asm volatile ("  jmp 0");
        break;  
      }
  }
  strip.setBrightness(neoBrightness);
  strip.show(); 
}

/*********************************************
Read bluetooth data 
*********************************************/
void getBluetooth() { 
  while(BTSerial.available() > 0)
  {
     char aChar = BTSerial.read();
     if(aChar == '\n')
     {
        // End of record detected. Time to parse and process
        String str(inData);
        int buttonInput;
        int buttonState;
        if (str.length() == 3) {
          if (varDebug) Serial << "IN: " << str.substring(0,1) << " " <<  str.substring(2,3) << " " << str.substring(4,5) << " " << str.substring(6,7) << " " << str.substring(8,9) << "\t";
          buttonInput = str.substring(0,1).toInt();
          buttonState = str.substring(2,3).toInt();
          if (varDebug) Serial << "= " << buttonInput << "," << buttonState << "\n\r";
          /*
           * 0 - Turn Left
           * 1 - Beam
           * 2 - Fog
           * 3 - Horn
           * 4 - Lights
           * 5 - Turn Right
           * 9 - Resend
           */
           changeWheelState(buttonInput, buttonState);
        } else {
          if (varDebug) Serial << "Invalid string length received " << inData << "(" << String(inData).length() << ") \n\r";
          BTSerial.flush();
        }
        index = 0;
        inData[index] = NULL;
     }
     else
     {
      // Prevent invalid characters being received
      if (aChar == '0' || aChar == '1' || aChar == '2' || aChar == '3' || aChar == '4' || aChar == '5' || aChar == '9' || aChar == ',' || aChar == '\n') {
        inData[index] = aChar;
      } else {
        inData[index] = '\n'; // Bad data, force end of String
        BTSerial.flush();
      }
      index++;
      inData[index] = '\0'; // Keep the string NULL terminated
     }
  }
}


void sendButton(int ButtonNum, int ButtonState){
  BTSerial << ButtonNum << "," << ButtonState << "\n";
  if(varDebug) Serial << "Out "<< ButtonNum << "," << ButtonState << "\n\r";
}

