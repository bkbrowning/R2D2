// R2I2CTransmitter.ino
// Wireless TRANSMITTER for R2D2 Platform

/*
* Brent K. Browning
* v1.1 - Added I2C handling routines for testing
* v1.2 - Added initial switch statement for servo instructions...but did not add servo library yet
* v2.0 - Modified for use with new radio setup
* v2.1 - Added keepAliveTime for monitoring the wireless connection; added LCD display routine
* v2.2 - Added new animations:  wave and sweep
* 
* Designed to work with Chris James' STEALTH controller for R2D2.
* Insert some setup notes on the radio, servo, etc. configurations
*/

#include <SPI.h>  // Serial communication protocol library
#include "RF24.h" // Library for the nRF24L01+ wireless radio
#include <Wire.h> // I2C communication protocol library
#include <Adafruit_GFX.h> // Library for LED display
#include <Adafruit_SSD1306.h> //Library for LED display

// set address definitions for my I2C network within R2
#define STEALTHI2C 0     // I2C address for the STEALTH controller...or its simululator
#define R2TransI2C 10    // I2C address for WIRELESS TRANSMITTER ARDUINO device <--THIS DEVICE
#define R2ReceiveI2C 11  // I2C address for the WIRELESS RECEIVER ARDUINO device
#define R2DomeBaseI2C 12 // I2C address for the DOME BASE SERVO controller
#define R2DomePiesI2C 13 // I2C address for the DOME PIES SERVO controller
#define R2BodyI2C 14     // I2C address for the BODY SERVO controller

//initialize the small SPI LCD
#define OLED_MOSI   4
#define OLED_CLK    3
#define OLED_DC     5
#define OLED_CS     7
#define OLED_RESET  6
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// setup the nRF24L01+ radio configuration
// NOTE:  this currently assumes a two radio setup.  Radio "1" is the R2TransI2C defined above.
//        Radio "0" is the R2ReceiveI2C.
bool radioNumber = 1;
RF24 radio(9,10);                        // radio(CE,CSN) on the SPI bus pins for the arduino...pins 9 & 10 in my setup
byte addresses[][6] = {"1Node","2Node"}; // address "codes" for the radios.  These should be unique and not easily guessed to avoid interception.  Must be identical for both radios.

// Global variables
int I2CInputCommand = 0;                   // holds currently processing I2C command
unsigned long startTime = 0;               // holds system time for calculating transmission times
int WIRELESSTRANSDELAY = 500;              // delay time in milliseconds between wireless transmissions
unsigned long keepAliveTime = 30000;       // time, in milliseconds, that the transmitter/receiver will wait for a signal before issuing it's own "ping"
                                           //   which is useful for ensuring that the wireless connection is still active even when no commands are being sent.
unsigned long keepAliveElapsed = millis(); // global elapsed time holder...which starts at current time
// Create a data structure for passing the I2C information via wireless
struct I2CTransStruct{
  int destinationI2Caddress;
  int I2CCommandValue;
  unsigned long time;
};
typedef struct I2CTransStruct I2CTrans;
I2CTrans I2CCommand;

// -----------------------------------------------
// Initial Setup Commands
// -----------------------------------------------
void setup() {
  // Initialize the serial port for status messaging
  Serial.begin(115200);
  Serial.println(F("R2I2C Transmitter - Started - v1.1"));

  // Intialize the I2C channel for this device
  Wire.begin(R2TransI2C);          // Start I2C Communication Bus at this address
  Wire.onReceive(receiveI2CEvent); // register i2c receive event which will be called when the I2C channel sees a command

  //
  // Begin the wireless radio
  //
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);   // setting the power on the radio to MAX value
  // Open a writing and reading pipe on each of the two radios, with opposite addresses
  if(radioNumber){
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1,addresses[0]);
  }else{
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1,addresses[1]);
  }
  // Start the radio listening for data
  radio.startListening();


  // initialize the display
  display.begin(SSD1306_SWITCHCAPVCC);
  display.display();
  delay(2000);
  display.clearDisplay();

  // initialize the display
  display.setTextSize(1);      // set the LCD's text size to the smallest value
  display.setTextColor(WHITE); // set text color to white (redundant for monochrome display, but works with other LCDs)
  display.setCursor(0,0);      // position the cursor pin to the top left corner of the LCD
  display.println("R2D2IC Transmitter");
  display.println("Row 2");
  display.println("Row 3");
  display.println("Row 4");
  display.display();
  delay(2000);
  display.clearDisplay();
}


// -----------------------------------------------
// Main Processing Loop
// -----------------------------------------------
void loop() {

  // monitor the elapsed time without a wireless event and fire a wireless signal after a keepAliveTime
  
  if ((keepAliveTime) <= (millis() - keepAliveElapsed)){
    // fire a wireless event and update the display
    Serial.println(F("KeepALive expired...sending ping to wireless..."));
    I2CWirelessRelay(R2DomeBaseI2C, 99);
    putLCDDisplay("99", "WIRELESS PING", "", keepAliveElapsed);
    // reset the timer to current time
    keepAliveElapsed = millis();
  }

} // end LOOP


// ////////////////////////////////////////////////////
// 
// FUNCTIONS
//
// ////////////////////////////////////////////////////

//-----------------------------------------------------
// commandHandler - process the received command
//-----------------------------------------------------
void commandHandler(int I2CCommandValue){
  switch(I2CCommandValue) {
    case 1: // reset signal received from STEALTH Controller
      Serial.println(F("Command 1; RESET STEALTH; Destination NONE;"));
      I2CInputCommand=-1;
      break;

    case 6: // body data panel open OR close
      Serial.println(F("Command 6; DATAPANEL o/c; Destination Body;"));
      startTime = millis();
      putLCDDisplay("6", "DATAPANEL o/c", "BODY", startTime);
      // sendI2CCommand(R2BodyI2C, "6");
      I2CInputCommand = -1;
      break;

    case 7: // body charging bay open OR close
      Serial.println(F("Command 7; CHARGEBAY o/c; Destination Body;"));
      startTime = millis();
      putLCDDisplay("7", "CHARGEBAY o/c", "BODY", startTime);
      // sendI2CCommand(R2BodyI2C, "7");
      I2CInputCommand = -1;
      break;

    case 8: // all panels open OR close
      Serial.println(F("Command 8; ALLPANELS o/c; Destination Body, DomeB, DomeP;"));
      startTime = millis();
      putLCDDisplay("8", "ALLPANELS o/c", "BODY + D-PAN + D-PIE", startTime);
      // sendI2CCommand(R2BodyI2C, "8");
      I2CWirelessRelay(R2DomeBaseI2C, 8);
      delay(WIRELESSTRANSDELAY);
      I2CWirelessRelay(R2DomePiesI2C, 8);
      keepAliveElapsed = millis();
      I2CInputCommand = -1;
      break;

    case 10: // all dome panels open OR close
      Serial.println(F("Command 10; DOMEPANELS o/c; Destination DomeB, DomeP;"));
      startTime = millis();
      putLCDDisplay("10", "DOMEPANELS o/c", "D-PAN + D-PIE", startTime);
      I2CWirelessRelay(R2DomeBaseI2C, 10);
      delay(WIRELESSTRANSDELAY);
      I2CWirelessRelay(R2DomePiesI2C, 10);
      keepAliveElapsed = millis();
      I2CInputCommand = -1;
      break;

    case 11: // right front body panel open OR close
      Serial.println(F("Command 11; RIFRBODY o/c; Destination Body;"));
      startTime = millis();
      putLCDDisplay("11", "RI FR BODY o/c", "BODY", startTime);
      //sendI2CCommand(R2BodyI2C, "11");
      I2CInputCommand = -1;
      break;

    case 12: // left rear body panel open OR close
      Serial.println(F("Command 12; LFRRBODY o/c; Destination Body;"));
      startTime = millis();
      putLCDDisplay("12", "LF RR BODY o/c", "BODY", startTime);
      //sendI2CCommand(R2BodyI2C, "12");
      I2CInputCommand = -1;
      break;

    case 13: // left front body panel open OR close
      Serial.println(F("Command 13; LFFRBODY o/c; Destination Body;"));
      startTime = millis();
      putLCDDisplay("13", "LF FR BODY o/c", "BODY", startTime);
      //sendI2CCommand(R2BodyI2C, "13");
      I2CInputCommand = -1;
      break;

    case 14: // all dome panels close
      Serial.println(F("Command 14; DOMEPANELS close; Destination DomeB, DomeP;"));
      startTime = millis();
      putLCDDisplay("14", "DOMEPANELS close", "D-PAN + D-PIE", startTime);
      I2CWirelessRelay(R2DomeBaseI2C, 14);
      delay(WIRELESSTRANSDELAY);
      I2CWirelessRelay(R2DomePiesI2C, 14);
      keepAliveElapsed = millis();
      I2CInputCommand = -1;
      break;

    case 15: // all dome panels WAVE
      Serial.println(F("Command 15; DOMEPANELS wave; Destination DomeB, DomeP;"));
      startTime = millis();
      putLCDDisplay("15", "DOMEPANELS wave", "D-PAN + D-PIE", startTime);
      I2CWirelessRelay(R2DomeBaseI2C, 15);
      delay(WIRELESSTRANSDELAY);
      I2CWirelessRelay(R2DomePiesI2C, 15);
      keepAliveElapsed = millis();
      I2CInputCommand = -1;
      break;

    case 16: // all dome panels sweep
      Serial.println(F("Command 16; DOMEPANELS sweep; Destination DomeB, DomeP;"));
      startTime = millis();
      putLCDDisplay("16", "DOMEPANELS sweep", "D-PAN + D-PIE", startTime);
      I2CWirelessRelay(R2DomeBaseI2C, 16);
      delay(WIRELESSTRANSDELAY);
      I2CWirelessRelay(R2DomePiesI2C, 16);
      keepAliveElapsed = millis();
      I2CInputCommand = -1;
      break;

    default:
    case 0:
      I2CInputCommand=-1;
      break;       
  } // end SWITCH
}


//-----------------------------------------------------
// I2CWirelessRelay - relay the I2C command via wireless radio
//-----------------------------------------------------
void I2CWirelessRelay(int destI2C, int destCommand){
  // relay the values via the radio
  radio.stopListening();  // pause the radio and prep for sending
  unsigned long beginTime = millis();
  I2CCommand = {destI2C, destCommand, beginTime};
  if (!radio.write( &I2CCommand, sizeof(I2CTransStruct) )){
    Serial.println(F("failed"));
  }
  radio.startListening();

  unsigned long started_waiting_at = beginTime;               // Set up a timeout period, get the current milliseconds
  boolean timeout = false;                                   // Set up a variable to indicate if a response was received or not
  
  while ( ! radio.available() ){                             // While nothing is received
    if ((millis()-started_waiting_at) > (1000)){            // If waited longer than 1000ms, indicate timeout and exit while loop
      timeout = true;
      break;
    }      
  }
        
  if ( timeout ){                                             // Describe the results
    Serial.println(F("Failed, response timed out."));
  } else {
    unsigned long got_time;                                 // Grab the response, compare, and send to debugging spew
    radio.read( &got_time, sizeof(unsigned long) );
    unsigned long currentTime = millis();
    //delay(1000);
    // Spew it
    Serial.print(F("Sent "));
    Serial.print(beginTime);
    Serial.print(F(", Got response "));
    Serial.print(got_time);
    Serial.print(F(", Round-trip delay "));
    Serial.print((currentTime-got_time));
    Serial.println(F(" milliseconds"));
  }
}

//-----------------------------------------------------
// i2c Command Event Handler
//-----------------------------------------------------

void receiveI2CEvent(int howMany) {
  Serial.print("Received I2C value of: ");
  //while (0 < Wire.available()) {
  //  byte c = Wire.read();
  //  Serial.print(c);  
  //}
  I2CInputCommand = Wire.read();
  Serial.println(I2CInputCommand);
  delay(1000);
  commandHandler(I2CInputCommand);
}

//----------------------------------------------------------------------------
// New i2c Commands
//----------------------------------------------------------------------------
void sendI2CCommand(int deviceID, String cmd) {
  byte sum = 0;
  Wire.beginTransmission(deviceID);
  for (int i=0;i<cmd.length();i++) {
    Wire.write(cmd[i]);
    sum+=byte(cmd[i]);
  }
  Wire.write(sum);
  Wire.endTransmission();  
}

//-------------------------------------------------------
// putLCDDisplay - renders the transmitter status message
//-------------------------------------------------------
//
// Format of LCD assumed to be:
//   LEN: 123456789012345678901
//   Ln1: IC2 Cmd: ###
//   Ln2: D: 123456789012345678
//   Ln3: BODY + D-PAN + D-PIE
//   Ln4: SentTm: XXXXXXXXXXXXX
void putLCDDisplay(String I2CValue, String commandDescription, String commandDestination, unsigned long sentTime){
  display.setTextSize(1);      // set the LCD's text size to the smallest value
  display.setTextColor(WHITE); // set text color to white (redundant for monochrome display, but works with other LCDs)
  display.setCursor(0,0);      // position the cursor pin to the top left corner of the LCD
  // Row 1
  display.print("IC2 Cmd: ");
  display.println(I2CValue);
  // Row 2
  display.print("D: ");
  display.println(commandDescription);
  // Row 3
  display.println(commandDestination);
  // Row 4
  display.print("SentTm: ");
  display.println(sentTime);
  display.display();
  display.clearDisplay();
}
