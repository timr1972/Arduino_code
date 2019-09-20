boolean varDebug = 0; // Set to 1 to force ignition on

/*
 * Input for the three feeds are via a 4 pin TLP621
 * LED in the Opto is 50mA
 * 12v supply, 2.2v drop = 220 Ohm 1 Watt resistor
 * 
 * Pin 2 - Not In Use
 * Pin 3 - ignitionKey - Voltage Divider, HIGH = Ignition on (22k/10k)
 * Pin 4 - Not in Use
 */

/*
 * varDebug
 * 0 = Normal Running
 * 1 = Serial Messages
 * 2 = Force ignition to On
 */

/****************** Loaded Modules ****************/
#include <Streaming.h>          // Used for the simple String concatination
#include <SoftwareSerial.h>
/****************** Static Values ****************/

/********************* Variable *******************/
const byte tachoInput     = 2;  // Not connected
const byte ignitionKey    = 3;  // This is interrupt 1 via Opto Coupler
const byte pin4           = 4;  // Not connected 
const byte btInput        = 5;  // Bluetooth connected indicator
const byte txPin          = 6;  // Bluetooth TX
const byte rxPin          = 7;  // Bluetooth RX
// Pin 8 NC
const int sidelight       = 9;
const int rightBeam       = 10;
const int rightHeadlight  = 12;
const int fog             = 11;
const byte btLED          = 13;
const int turnright       = 18;
const int leftHeadlight   = 15; 
const int leftBeam        = 16;
const int horn            = 17;
const int turnleft        = 14; 

char inData[10];
byte index = 0;

const byte numberOfButtons= 6;

int buttonState[]         = {1,1,1,1,1,1};
int oldButtonState[]      = {1,1,1,1,1,1};

byte ignitionStatus       = 0; 
int beamLongPress         = 250;
long keepAlive            = 10000;
long startBtnCounter      = 0;
long flashExpire          = 0;
const int flashTurnTime   = 3000;

long beamOnTime = 0;
long time;
long time_last;

/**************** Serial Ports ******************/
SoftwareSerial BTSerial(rxPin, txPin);

/********************* Easy Transfer *********************/
struct RECEIVE_DATA_STRUCTURE{
  byte turnleftButton = 0;
  byte beamButton     = 0;
  byte fogButton      = 0;
  byte lightButton    = 0;
  byte turnrightButton= 0;
  byte hornButton     = 0;
  byte specialButton  = 0;
};

struct SEND_DATA_STRUCTURE{
  byte turnleftState  = 0;
  byte beamState      = 0;
  byte fogState       = 0;
  byte lightsState    = 0;
  byte turnrightState = 0;
  byte hornState      = 0;
  byte specialButton  = 0;
};

RECEIVE_DATA_STRUCTURE receivedata;
SEND_DATA_STRUCTURE senddata;

struct OLD_DATA_STRUCTURE{
  byte turnleftButton  = receivedata.turnleftButton;
  byte beamButton      = receivedata.beamButton;
  byte fogButton       = receivedata.fogButton;
  byte lightButton     = receivedata.lightButton;
  byte turnrightButton = receivedata.turnrightButton;
  byte hornButton      = receivedata.hornButton;
  byte specialButton   = receivedata.specialButton;
};
OLD_DATA_STRUCTURE oldData;
/********************* Easy Transfer *********************/

/*********************************************/
/*                                           */
/*            Startup routine                */
/*                                           */
/*********************************************/
void setup() {
  pinMode(turnright, OUTPUT);
  digitalWrite(turnright, HIGH);
  
  pinMode(leftHeadlight, OUTPUT);
  digitalWrite(leftHeadlight, HIGH);
  
  pinMode(leftBeam, OUTPUT);
  digitalWrite(leftBeam, HIGH);
  
  pinMode(horn, OUTPUT);
  digitalWrite(horn, HIGH);
  
  pinMode(rightBeam, OUTPUT);
  digitalWrite(rightBeam, HIGH);
  
  pinMode(fog, OUTPUT);
  digitalWrite(fog, HIGH);
  
  pinMode(turnleft, OUTPUT);
  digitalWrite(turnleft, HIGH);
  
  pinMode(rightHeadlight, OUTPUT);
  digitalWrite(rightHeadlight, HIGH);
  
  pinMode(sidelight, OUTPUT);
  digitalWrite(sidelight, HIGH);
  
  pinMode(btLED, OUTPUT);
  
  pinMode(tachoInput, INPUT_PULLUP);
  pinMode(ignitionKey, INPUT);
  pinMode(pin4, INPUT_PULLUP);
  pinMode(btInput, INPUT);

  Serial.begin(115200);
  BTSerial.begin(9600);

  /*
   * ~5s startup delay
   */
  for (int i=0; i<10; i++) {
    digitalWrite(btLED, HIGH);
    delay(100);
    digitalWrite(btLED, LOW);
    delay(100);
  }
  
  /*
   * Reset BlueTooth serial input
   */
  BTSerial.flush();
  
  /*
   * Reset LED's on wheel
   */
  for (int i=0; i<6; i++) {
    BTSerial << i;
    delay(5);  
    BTSerial << ",";
    delay(5);  
    BTSerial << "0";
    delay(5);  
    BTSerial << "\n";
    delay(20);
  }
  Serial << "V8_Dashboard is up " << varDebug << "\n";
}

/*********************************************/
/*                                           */
/*                Main Loop                  */
/*                                           */
/*********************************************/
void loop() {
  /*
  * Check for any input from the steering wheel
  */ 
  getBluetooth();

  /*
   * Handle any button change information we have received
   */
  if (receivedata.turnleftButton != oldData.turnleftButton) { // Turn Left
    changeWheelState(0, receivedata.turnleftButton);
    oldData.turnleftButton = receivedata.turnleftButton;
  }
  if (receivedata.beamButton != oldData.beamButton) { // Beam
    changeWheelState(1, receivedata.beamButton);
    oldData.beamButton = receivedata.beamButton;
  }
  if (receivedata.fogButton != oldData.fogButton) { // Fog
    changeWheelState(2, receivedata.fogButton);
    oldData.fogButton = receivedata.fogButton;
  }
  if (receivedata.hornButton != oldData.hornButton) { // Horn
    changeWheelState(3, receivedata.hornButton);
    oldData.hornButton = receivedata.hornButton;
  }
  if (receivedata.lightButton != oldData.lightButton) { // Lights
    changeWheelState(4, receivedata.lightButton);
    oldData.lightButton = receivedata.lightButton;
  }
  if (receivedata.turnrightButton != oldData.turnrightButton) { // Turn Right
    changeWheelState(5, receivedata.turnrightButton);
    oldData.turnrightButton = receivedata.turnrightButton;
  }

  if (receivedata.specialButton != oldData.specialButton) { // Turn Right
    changeWheelState(9, receivedata.specialButton);
    oldData.specialButton = receivedata.specialButton;
  }
    
  /*
  * Check for a change in the ignition key status, turned on or off
  */
  if (varDebug != 2) {
    if(digitalRead(ignitionKey)==HIGH and ignitionStatus == 0) ignitionSwitchOn();
    if(digitalRead(ignitionKey)==LOW and ignitionStatus == 1) ignitionSwitchOff();
  } else {
    ignitionStatus = 1;
  }
  
  /*
  * If the ignition is on, run the flash  check for the indicators
  */
  if(ignitionStatus) flashTimer();

  /*
   * Check for Bluetooth Connection
   */  
  if(digitalRead(btInput) == HIGH) {
    digitalWrite(btLED, LOW);
  } else {
    digitalWrite(btLED, HIGH); // No bluetooth link so light the warning lamp.
  }
} 
/******************* End of Main Loop ********************/


/*********************************************
Read bluetooth data 
*********************************************/
void getBluetooth() { 
  while(BTSerial.available() > 0)
  {
     char aChar = BTSerial.read();
     if(aChar == '\n') {
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
           switch(buttonInput){
            case 0:
            receivedata.turnleftButton = buttonState;
            break;
            case 1:
            receivedata.beamButton = buttonState;
            break;
            case 2:
            receivedata.fogButton = buttonState;
            break;
            case 3:
            receivedata.hornButton = buttonState;
            break;
            case 4:
            receivedata.lightButton = buttonState;
            break;
            case 5:
            receivedata.turnrightButton = buttonState;
            break;
            case 9:
            receivedata.specialButton = buttonState;
            break;
           }
        } else {
          if  (varDebug) Serial << "Invalid string length received " << inData << "(" << String(inData).length() << ") \n\r";
          BTSerial.flush();
        }
        index = 0;
        inData[index] = NULL;
     } else {
      // Prevent invalid characters being received
      if (aChar == '0' || aChar == '1' || aChar == '2' || aChar == '3' || aChar == '4' || aChar == '5' || aChar == '9' || aChar == ',' || aChar == '\n') {
        inData[index] = aChar;
        // if(varDebug) Serial << aChar;
      } else {
        inData[index] = '\n'; // Bad data, force end of String
        BTSerial.flush();
      }
      index++;
      inData[index] = '\0'; // Keep the string NULL terminated
     }
  }
}

/*********************************************
 Ignition Turned On
*********************************************/
void ignitionSwitchOn() {
  //
  // Check to see if this is actually a physical change of state
  //
  if(digitalRead(ignitionKey) == HIGH and ignitionStatus == 0) {
    if (varDebug) Serial << "Ignition Turned On \n\r";
    keepAlive        = 1000;       // Increase keep alives to the steering wheel
    ignitionStatus   = 1;          // Change ignition state
  }
}

/*********************************************
  Ignition Turned Off  
*********************************************/
void ignitionSwitchOff() {
  //
  // Check to see if this is actually a physical change of state
  //
  if(digitalRead(ignitionKey) == LOW and ignitionStatus == 1) {
    if (varDebug) Serial << "Ignition Turned Off \n\r";
    keepAlive       = 3000;  // Reduce keep alives to the steering wheel
    ignitionStatus  = 0;     // Change ignition state
    
    //
    // Cancel Left Turn
    //
    if (senddata.turnleftState != 0) {
      senddata.turnleftState = 0;
      digitalWrite(turnleft, HIGH);  // Turn Left Off
      BTSerial << "0,0\n";
      delay(50);
    }
    
    //
    // Cancel Beam
    //
    if (senddata.beamState != 0) {
      digitalWrite(leftBeam, HIGH);  // Beam L Off
      digitalWrite(rightBeam, HIGH);  // Beam R Off
      senddata.beamState = 0; // Set Beam Off state
      if(varDebug) Serial << "Beam Off\n\r";
      BTSerial << "1,0\n";
      delay(50);
      senddata.lightsState = 1;
      BTSerial << "4,1\n";
      delay(50);
    } else {
      //
      // Cancel Headlights
      //
      if (senddata.lightsState == 2 )  {
        senddata.lightsState = 1;      // If lights were on, take them back to side only
        digitalWrite(leftHeadlight, HIGH);  // Head L Off
        digitalWrite(rightHeadlight, HIGH);  // Head R Off
        if(varDebug) Serial << "Headlights Off\n\r";
        BTSerial << "4,1\n";
        delay(50);
      }
    }

    //
    // Cancel Right Turn
    //
    if (senddata.turnrightState != 0) {
      senddata.turnrightState = 0;
      digitalWrite(turnright, HIGH);  // Turn Right Off
      BTSerial << "5,0\n";
      delay(50);
    }
    flashExpire = 0;
  }
}

/*********************************************
  Check for  on the turning signals 
*********************************************/
void flashTimer() { 
    if (senddata.turnleftState==1 and millis() > flashExpire) {   // Turn Left expired, cancel light and dash
      digitalWrite(turnleft, HIGH);  // Turn Left Off
      senddata.turnleftState = 0;                                 // Update State
      BTSerial << "0,0\n";
    }
    if (senddata.turnrightState == 1 and millis() > flashExpire) { // Turn Right expired, cancel light and dash
      digitalWrite(turnright, HIGH);  // Turn Right Off
      senddata.turnrightState = 0;                                 // Update State
      BTSerial << "5,0\n";
    }  
}

/*********************************************
  We've received a state change, do something
*********************************************/
void changeWheelState(int button, int state) {
  digitalWrite(btLED, HIGH);
  if (varDebug > 0) Serial << "Rcvd Change Wheel State: " << button << "," << state << " Ignition: " << ignitionStatus << "\n\r";
  switch (button) {
    /*
    * Turn Left
    * 
    * relay1 |= (1 << 0);  // Turn Left On
    * relay1 &= ~(1 << 0);  // Turn Left Off
    * 
    */
    case 0:
      if (ignitionStatus == 1 and state == 0) {
        // Turn off right turn if it's on
        if (senddata.turnrightState != 0) {
          senddata.turnrightState=0;                 // Right Turn state to Off
          digitalWrite(turnright, HIGH);
          BTSerial << "5,0\n";
          delay(50);
        } 
        switch (senddata.turnleftState){             // This is the Left Turn State: 0,1,2
          case 0:
            senddata.turnleftState = 1;              // Left Turn State update
            flashExpire = millis() + flashTurnTime;  // Set the Timer
            digitalWrite(turnleft, LOW);             // Turn Left On
            BTSerial << "0,1\n";
            delay(50);
            break;
          case 1:
            senddata.turnleftState = 2;              // Change lights on the wheel
            BTSerial << "0,2\n";
            delay(50);
            flashExpire = 0;                         // Cancel auto expire for the turn signal
            break;
          case 2:
            senddata.turnleftState = 0;              // Change lights on the wheel
            flashExpire = 0;                         // Cancel auto expire for the turn signal
            digitalWrite(turnleft,HIGH);             // Turn Left Off
            BTSerial << "0,0\n";
            delay(50);
            break;
        }
      }
      break;
    
    /*
    * Beam
    * 
    */  
    
    case 1: 
      if(state == 1) {                                        // Button pressed
        if(senddata.beamState == 1) {                         // Beam state 0,1
          digitalWrite(leftBeam, HIGH);                       // Beam L Off
          digitalWrite(rightBeam, HIGH);                      // Beam R Off
          digitalWrite(leftHeadlight, LOW);                   // Headlight L On
          digitalWrite(rightHeadlight, LOW);                  // Headlight R On
          senddata.beamState = 0;                             // Update Beam state 
          BTSerial << "1,0\n";
          if(varDebug) Serial << "1,0\n\r";
        } else {
          // Switch Beam on
          digitalWrite(leftBeam, LOW);                        // Beam L On
          digitalWrite(rightBeam, LOW);                       // Beam R On
          senddata.beamState = 2;                             // Update Beam State
          beamOnTime = millis() + beamLongPress;              // Register press time so we can measure if this is a long or short press
          BTSerial << "1,1\n";
          if(varDebug) Serial << "1,1\n\r";
        }
      }
      if(state == 0) { 
        // Button Released
        if (millis() > beamOnTime and senddata.beamState == 2 and senddata.lightsState == 2) {
          digitalWrite(leftBeam, LOW);                        // Beam L On
          digitalWrite(rightBeam, LOW);                       // Beam R On
          digitalWrite(leftHeadlight, HIGH);                  // Headlight L Off
          digitalWrite(rightHeadlight, HIGH);                 // Headlight R Off
          senddata.beamState = 1;
          BTSerial << "1,1\n";
          if(varDebug) Serial << "1,1\n\r";
        } else {
          // Beam off
          digitalWrite(leftBeam, HIGH);                       // Beam L Off
          digitalWrite(rightBeam, HIGH);                      // Beam R Off
          senddata.beamState = 0;                             // Update Beam Status
          BTSerial << "1,0\n";
          if(varDebug) Serial << "1,0\n\r";
        }
      }
      break;
    
    /*
    * Fog
    *
    */  
    case 2:
      if(state){
        if (senddata.lightsState !=0) {   // Check if lights are on and fog is on
          switch (senddata.fogState) {
            case 0:
              digitalWrite(fog, LOW);     // Fog On
              senddata.fogState = 1;
              BTSerial << "2,1\n";
              break;
            case 1:
              digitalWrite(fog, HIGH);    // Fog Off
              senddata.fogState = 0;
              BTSerial << "2,0\n";
              break;
            default:
              // Serial << "*** Bad Fog status\n";
              break;
          }
        } else {
          if (senddata.fogState == 1) {
              digitalWrite(fog, HIGH);    // Fog Off
              senddata.fogState = 0;
              BTSerial << "2,0\n";
          }
        }
      }
      break;

    /*
    * Horn
    */
    case 3:
      if(state and ignitionStatus) {
        // Horn Relay on
        digitalWrite(horn, LOW);          // Horn On
      } else {
        // Horn Relay off
        digitalWrite(horn, HIGH);         // Horn Off
      }
      break;
      
    /*
    * Lights
    */
    case 4:
      if(state == 0) break;
      switch(senddata.lightsState) {
        case 0: 
        // Turn Side lights on
          digitalWrite(sidelight, LOW);     // Side Lights Right on
          senddata.lightsState = 1;
          if (varDebug > 0) Serial << "Out 4,1\n\r";
          BTSerial << "4,1\n";
          break;
        case 1: 
        // Head lights (Off if ignition is off)
          if(!ignitionStatus) {                 // Ignition off, so lights off
            digitalWrite(sidelight, HIGH);      // Side Lights Right off
            if (senddata.fogState != 0) {
              digitalWrite(fog, HIGH);          // Fog Off
              senddata.fogState = 0;            // Fogs state off
              if (varDebug > 0) Serial << "Out 2,0\n\r";
              BTSerial << "2,0\n\n";
              delay(50);
            }
            senddata.lightsState = 0;           // Side off
            if (varDebug > 0) Serial << "Out 4,0\n\r";
            BTSerial << "4,0\n";
          } else {                              // Ignition On
            digitalWrite(leftHeadlight, LOW);   // Headlight L On
            digitalWrite(rightHeadlight, LOW);  // Headlight R On
            senddata.lightsState = 2;
            if (varDebug > 0) Serial << "Out 4,2\n\r";
            BTSerial << "4,2\n";
            delay(50);
          }
          break;
        case 2: // Back to Side
          if (senddata.beamState != 0) {
            digitalWrite(leftBeam, HIGH);       // Beam L Off
            digitalWrite(rightBeam, HIGH);      // Beam R Off
            senddata.beamState = 0;             // Update Beam Status
            if (varDebug > 0) Serial << "Out 1,0\n\r";
            BTSerial << "1,0\n";
            delay(50);
          }
          digitalWrite(leftHeadlight, HIGH);    // Headlight L Off
          digitalWrite(rightHeadlight, HIGH);   // Headlight R Off
          if (varDebug > 0) Serial << "Out 4,3\n\r";
          BTSerial << "4,3\n";
          delay(50);
          senddata.lightsState = 3;
          break;
        case 3: // Lights Off
          digitalWrite(sidelight, HIGH);        // Side Lights Right off
          senddata.lightsState = 0;             // Lights Off
          if (varDebug > 0) Serial << "Out 4,0\n\r";
          BTSerial << "4,0\n";
          delay(50);
          if (senddata.fogState != 0) {
            if (varDebug > 0) Serial << "Out 2,0\n\r";
            BTSerial << "2,0\n";
            delay(50);
            digitalWrite(fog, HIGH);            // Fog Off
            senddata.fogState = 0;              // Fogs state off
          }
          break;
        default:
          if(varDebug) Serial << "*** Error in light status\n\r";
          break;
      }
      break;

    /*
    * Turn Right
    */
    case 5: 
      if (ignitionStatus == 1 and state == 0) {
        if (senddata.turnleftState != 0) {
          senddata.turnleftState=0;             // Turn Left On Turn it off
          BTSerial << "0,0\n";
          delay(50);
          digitalWrite(turnleft, HIGH);         // Turn Left Off
        }
        switch (senddata.turnrightState){
          case 0:
            senddata.turnrightState = 1;
            flashExpire = millis() + flashTurnTime;
            digitalWrite(turnright, LOW);       // Turn Right ON
            BTSerial << "5,1\n";
            delay(50);
            break;
          case 1:
            senddata.turnrightState = 2;
            BTSerial << "5,2\n";
            delay(50);
            flashExpire = 0;
            break;
          case 2: // Cancel turn Right
            senddata.turnrightState = 0;
            flashExpire = 0;
            digitalWrite(turnright, HIGH);      // Turn Right Off
            BTSerial << "5,0\n";
            delay(50);
            break;
        }
      }
      break;
    case 9: 
      switch(receivedata.specialButton){
        case 2:  
        // Send Go sign to steering wheel.
          BTSerial << "9,1\n";
          delay(250);
          BTSerial << "9,0\n";
          break;
        case 3:
          // Removed for now

          BTSerial << "9,1\n";
          delay(50);
          int ssf = 100;
          int lsf = 250;
          /*            
           // Flash Left
              relay1 |= (1 << 2);   // Beam 1 On
              relay2 &= ~(1 << 5);  // Beam 2 Off
              updateRelays();
              delay(ssf);
              relay1 &= ~(1 << 2);  // Beam 1 Off
              updateRelays();
              delay(ssf);
              relay1 |= (1 << 2);   // Beam 1 On
              updateRelays();
              delay(ssf);
              relay1 &= ~(1 << 2);  // Beam 1 Off
              updateRelays();
              delay(lsf);
              
          // Flash Right
              relay2 |= (1 << 5);   // Beam 2 On
              updateRelays();
              delay(ssf);
              relay2 &= ~(1 << 5);  // Beam 2 Off
              updateRelays();
              delay(ssf);
              relay2 |= (1 << 5);   // Beam 2 On
              updateRelays();
              delay(ssf);
              relay2 &= ~(1 << 5);  // Beam 2 Off
              updateRelays();
              delay(lsf);
          // }
          */
          BTSerial << "9,0\n";
          delay(50);
          /*
          if (senddata.beamState == 2) {   // Short Beam On
            senddata.beamState = 1;        // Change to Long Beam
          }
          if (senddata.beamState == 1) {   // Long Beam On
            if (vardebug) Serial << "Beam State = 1 (Exit)\n\r";
            relay1 = relay1 ^ headlight1;  // Head Lights Left OFF
            relay2 = relay2 ^ headlight2;  // Head Lights Right OFF
            relay1 = relay1 | beam1;       // Beam Left ON
            relay2 = relay2 | beam2;       // Beam right ON
            updateRelays();
          }
          if (vardebug) Serial << "Head State != 2 (Exit)\n\r";
            relay1 = relay1 ^ headlight1;  // Head Lights Left OFF
            relay2 = relay2 ^ headlight2;  // Head Lights Right OFF
            updateRelays();   
          }
          */
          receivedata.specialButton = 0;
          break;
      }
    default:
      // Do nothing
      break;
  }
  digitalWrite(btLED, LOW);
}

/**********************************************************/
/* This is called to send messages of up to 30 characters */
/* But these are only sent if the debug flag is on        */
/**********************************************************/
void debug(String msg, int level) {     // Maximum message size is 30 characters
  if (varDebug > level) {
    Serial << (millis()) << ": " << msg << "\n\r";                  // Debug only sent to Physical Serial, not Bluetooth
  }
  return;
}


