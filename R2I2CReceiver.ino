// R2I2CReceiver.ino
// Wireless RECEIVER for R2D2 Platform

/*
* Brent K. Browning
* v1.1 - Initial configuration for testing
* v1.2 - Updated LCD display functionality
* v2.0 - Change I2C handling to work with integers
* 
* - Designed to work with Chris James' STEALTH controller for R2D2.
* - Uses the nRF24L01 receiver/transmitter to create a wireless signal bridge.  Specific application for this
*   setup is to act as wireless I2C bridge between the body and dome of an R2 unit.  I2C signals are received on
*   the companion transmitter and then are converted to wireless signal for broadcast to this receiver.
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
#define OLED_MOSI  4
#define OLED_CLK   3
#define OLED_DC    5
#define OLED_CS    7
#define OLED_RESET 6
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// setup the nRF24L01+ radio configuration
// NOTE:  this currently assumes a two radio setup.  Radio "1" is the R2TransI2C defined above.
//        Radio "0" is the R2ReceiveI2C.
bool radioNumber = 0;
RF24 radio(9,10);                        // radio(CE,CSN) on the SPI bus pins for the arduino...pins 9 & 10 in my setup
byte addresses[][6] = {"1Node","2Node"}; // address "codes" for the radios.  These should be unique and not easily guessed to avoid interception.  Must be identical for both radios.

// Create a data structure for passing the I2C information via wireless
struct I2CTransStruct{
  int destinationI2Caddress;
  int I2CCommandValue;
  unsigned long sentTime;
};
typedef struct I2CTransStruct I2CTrans;
I2CTrans got_data;

char _int2str[7];

// counter for LCD testing
unsigned long counter = 0;

void setup() {
  // Initialize the serial console for this device
  // NOTE:  This section may be removed for 'production' version once debugging is complete
  Serial.begin(115200); // baud rate for the serial console
  Serial.println("R2 Signal Receiver is initializing...");
  Serial.print("I2C address for this device is ");
  Serial.println(R2ReceiveI2C);

  // initialize the display
  display.begin(SSD1306_SWITCHCAPVCC);
  display.display();
  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);      // set the LCD's text size to the smallest value
  display.setTextColor(WHITE); // set text color to white (redundant for monochrome display, but works with other LCDs)
  display.setCursor(0,0);      // position the cursor pin to the top left corner of the LCD
  display.println("R2D2IC Receiver");
  display.println("Row 2");
  display.println("Row 3");
  display.println("Row 4");
  display.display();
  delay(2000);
  display.clearDisplay();

  // Intialize the I2C channel for this device
  Wire.begin(R2ReceiveI2C);   // Start I2C Communication Bus as a Slave

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
}

void loop() {
  if( radio.available()){
    while (radio.available()) {                                   // While there is data ready
      radio.read( &got_data, sizeof(I2CTransStruct) );             // Get the payload
    }
    // show message on serial port
    Serial.print("Got data!  Destination address:");
    Serial.print(got_data.destinationI2Caddress);
    Serial.print(" Destination value:");
    Serial.print(got_data.I2CCommandValue);
    Serial.print(" Received time:");
    Serial.println(got_data.sentTime);

    // relay received data down I2C path EXCEPT for where I2CCommandValue is the keepAlive value
    if (got_data.I2CCommandValue != 99){
      // this is a legitimate I2C command so relay it along
      sendI2CCommand(got_data.destinationI2Caddress, got_data.I2CCommandValue);
      putLCDDisplay(int2str(got_data.destinationI2Caddress), int2str(got_data.I2CCommandValue), "I2C RELAY", got_data.sentTime);
    } else {
      // this is the wireless keepAlive ping
      putLCDDisplay(int2str(got_data.destinationI2Caddress), int2str(got_data.I2CCommandValue), "WRLSS KEEPALIVE", got_data.sentTime);
    }

    // send a REPLY signal
    radio.stopListening();                                        // First, stop listening so we can talk   
    radio.write( &got_data.sentTime, sizeof(unsigned long) );     // Send the final one back.      
    radio.startListening();                                       // Now, resume listening so we catch the next packets.     
    Serial.print("Sent response: ");
    Serial.println(got_data.sentTime);
  } else {
    // do nothing...radio not ready
  }

} // Loop

// ////////////////////////////////////////////////////
// 
// FUNCTIONS
//
// ////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// sendI2CCommand - relays the incoming command down the I2C path to slaves
//----------------------------------------------------------------------------
void sendI2CCommand(int deviceID, int cmd) {
  // Send two bytes to slave.
  Wire.beginTransmission(deviceID);
  Wire.write(cmd >> 8);
  Wire.write(cmd & 255);
  Wire.endTransmission();

  Serial.print("Sent IC2 address:");
  Serial.print(deviceID);
  Serial.print("  Sent command:");
  Serial.println(cmd);  
}

//-------------------------------------------------------
// putLCDDisplay - renders the transmitter status message
//-------------------------------------------------------
//
// Format of LCD assumed to be:
//   LEN: 123456789012345678901
//   Ln1: IC2 Cmd: ###
//   Ln2: D: 123456789012345678
//   Ln3: 
//   Ln4: SentTm: XXXXXXXXXXXXX
void putLCDDisplay(String I2CValue, String commandDescription, String commandDestination, unsigned long sentTime){
  display.setTextSize(1);      // set the LCD's text size to the smallest value
  display.setTextColor(WHITE); // set text color to white (redundant for monochrome display, but works with other LCDs)
  display.setCursor(0,0);      // position the cursor pin to the top left corner of the LCD
  // Row 1
  display.print("IC2 Add: ");
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

//-----------------------------------------------------------
// int2str - covnerts INTEGER values into STRINGS for display
//-----------------------------------------------------------
char* int2str( register int i ) {
  register unsigned char L = 1;
  register char c;
  register boolean m = false;
  register char b;  // lower-byte of i
  // negative
  if ( i < 0 ) {
    _int2str[ 0 ] = '-';
    i = -i;
  }
  else L = 0;
  // ten-thousands
  if( i > 9999 ) {
    c = i < 20000 ? 1
      : i < 30000 ? 2
      : 3;
    _int2str[ L++ ] = c + 48;
    i -= c * 10000;
    m = true;
  }
  // thousands
  if( i > 999 ) {
    c = i < 5000
      ? ( i < 3000
          ? ( i < 2000 ? 1 : 2 )
          :   i < 4000 ? 3 : 4
        )
      : i < 8000
        ? ( i < 6000
            ? 5
            : i < 7000 ? 6 : 7
          )
        : i < 9000 ? 8 : 9;
    _int2str[ L++ ] = c + 48;
    i -= c * 1000;
    m = true;
  }
  else if( m ) _int2str[ L++ ] = '0';
  // hundreds
  if( i > 99 ) {
    c = i < 500
      ? ( i < 300
          ? ( i < 200 ? 1 : 2 )
          :   i < 400 ? 3 : 4
        )
      : i < 800
        ? ( i < 600
            ? 5
            : i < 700 ? 6 : 7
          )
        : i < 900 ? 8 : 9;
    _int2str[ L++ ] = c + 48;
    i -= c * 100;
    m = true;
  }
  else if( m ) _int2str[ L++ ] = '0';
  // decades (check on lower byte to optimize code)
  b = char( i );
  if( b > 9 ) {
    c = b < 50
      ? ( b < 30
          ? ( b < 20 ? 1 : 2 )
          :   b < 40 ? 3 : 4
        )
      : b < 80
        ? ( i < 60
            ? 5
            : i < 70 ? 6 : 7
          )
        : i < 90 ? 8 : 9;
    _int2str[ L++ ] = c + 48;
    b -= c * 10;
    m = true;
  }
  else if( m ) _int2str[ L++ ] = '0';
  // last digit
  _int2str[ L++ ] = b + 48;
  // null terminator
  _int2str[ L ] = 0;  
  return _int2str;
}
