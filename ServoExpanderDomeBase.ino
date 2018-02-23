// -----------------------------------------------
// ServoExpanderDomeBase
// Brent Browning
// v3.1 - 2018-02 - Fixed byte handler for receiving integer on I2C bus
// v3.2 - 2018-02 - Added wavePanels animation feature
// v3.3 - Added delayed startup
// -----------------------------------------------

// Includes
#include <VarSpeedServo.h>  // Variable speed server library.  SOURCE FOR FILE:  https://github.com/netlabtoolkit/VarSpeedServo
#include <Wire.h> // Arduino standard communication library for I2C

// -------------------------------------------------
// Define some constants to help reference objects,
// pins, servos, leds etc by name instead of numbers
// -------------------------------------------------
#define STATUS_LED 13 // Our Status LED on Arduino Digital IO #13, this is the built in LED on the Pro Mini
                      // NOTE: Be sure to use pin 13 with caution as it can sometimes be shared with a live servo, etc. and not be just the LED
                      
// Define and initialize the array to hold all of our servo data for servos connected TO THIS CONTROLLER
//    The array below is of the format...
//    {ArduinoPinNum, ServoClosedPosition, ServoOpenPosition, ServoCloseSpeed, ServoOpenSpeed}
#define NUMBER_OF_SERVOS 8 // total number of servos in the servo array below
VarSpeedServo Servos[NUMBER_OF_SERVOS];  // array to link all servos to the VarSpeedServo library
int myServos[NUMBER_OF_SERVOS][5] = {{ 2, 175, 40, 100, 100 },
                                     { 3, 175, 40, 100, 100 },
                                     { 4, 175, 40, 100, 100 },
                                     { 5, 40, 175, 100, 100 },
                                     { 6, 160, 60, 100, 100 },
                                     { 7, 175, 40, 100, 100 },
                                     { 8, 175, 40, 100, 100 },
                                     { 9, 175, 40, 100, 100 }};
// To make referring to our panels easier, we'll give them some more human-friendly names
// Note that the references below are to the servo array ROWS.  Should be one of these for each of NUMBER_OF_SERVOS
#define PANEL1 0
#define PANEL2 1
#define PANEL3 2
#define PANEL4 3
#define PANEL7 4
#define PANEL10 5
#define PANEL11 6
#define PANEL13 7
// And we will define some constants that we can use to make referring to the servo array COLUMNS in the array above easier to remember
#define PIN_NUM 0
#define CLOSED_POS 1
#define OPEN_POS 2
#define CLOSE_SPEED 3
#define OPEN_SPEED 4

// Other local variables
int i2cCommand = 0; // holds the current I2C command that is passed into the switch statement in the loop function below
int myi2c = 12; // The i2c address for this servo expander!  Important that this be unique.  If you have more than one servo expander, be sure to change this variable
boolean allPanelsOpen=false; // boolean flag to hold the current state of all panels;  assumes they are closed to start

// ----------------------------------------------------------
// Setup for Arduino - called once on boot
// ----------------------------------------------------------
void setup() {
  // the initial power draw on the R2D2 dome seems to cause an issue with this Arduino...
  // ...it eventually resets, but hoping this delay will help
  delay(10000);  // a 10 second bootup delay before this arduino and servos start to fire up
  
  // setup communications to the serial debug console and report some data
  // NOTE:  these "Serial.print" statements can be removed if no monitoring is needed
  Serial.begin(115200); // Debug Console
  Serial.println("STEALTH Servo Expander initializing");
  Serial.print("I2C address of this controller is ");
  Serial.println(myi2c);

  // Initialize the I2C Controller
  Wire.begin(myi2c);   // Start I2C Communication Bus as a Slave
  Wire.onReceive(receivei2cEvent); // register i2c receive event which will be called when we execute a command

  // Turn off Status LED
  pinMode(STATUS_LED, OUTPUT);  // set the mode for that pin as output
  digitalWrite(STATUS_LED, LOW); // set the LED off
  
  // Activate/Center all the servos to CLOSED position
  Serial.print("Activating and centering all servos...");
  for( int i = 0; i <  NUMBER_OF_SERVOS; i++) {
    Servos[i].attach(myServos[i][PIN_NUM]);
    Servos[i].write(myServos[i][CLOSED_POS],myServos[i][CLOSE_SPEED]);
    delay(1000);  // pause 1 second to allow for close to finish
    Servos[i].detach(); //Detach from the Servos to save power and cut potential jitter/hum.
  }
  Serial.println("Setup complete.  Waiting for i2c command.");
}


// -----------------------------------------------------
// -----------------------------------------------------
// Main Arduino Processing Loop
// -----------------------------------------------------
// -----------------------------------------------------
void loop() {
  delay(60);
  
  switch(i2cCommand) {
    case 1:
          Serial.println("Received reset message from I2C source");
          i2cCommand=-1;
          break;

    case 10:
          ToggleAllPanels();
          break;

    case 14:
          CloseAllPanels();
          break;

    case 15:
          WavePanels();
          break;

    case 16:
          SweepPanels();
          break;
         
    case 55: 
          OpenCloseSinglePanel();
          break;

    default:
    case 0:
          i2cCommand=-1; // "clear" the i2cCommand variable
          break;
  } // end SWITCH
} // end LOOP


// -----------------------------------------------------
// Open/Close a single panel for testing
// -----------------------------------------------------
void OpenCloseSinglePanel() {

    int PANELACTION = PANEL13; // SET THE PANEL# THAT NEEDS TESTING!
    
    digitalWrite(STATUS_LED, HIGH); // turn on STATUS LED so we can visually see we got the command on the board     
   
    Serial.print("Single Panel test: ");
   
    if (allPanelsOpen) { // Close the panels
      Serial.println("Closing panels...");

      // Attach to the panels so we can move them
      Servos[PANELACTION].attach(myServos[PANELACTION][PIN_NUM]);
      
      // Close the panels
      Servos[PANELACTION].write(myServos[PANELACTION][CLOSED_POS],myServos[PANELACTION][CLOSE_SPEED]);
      Serial.println("Panel closed to position " + String(myServos[PANELACTION][CLOSED_POS]));

      // Detach from the panel servos    
      delay(1500);
      Servos[PANELACTION].detach();
  
      Serial.println("Panels closed!");
      allPanelsOpen=false;
       
    } else { // Open the panels
      Serial.println("Opening panels...");

      // Attach to the panels so we can move them
      Servos[PANELACTION].attach(myServos[PANELACTION][PIN_NUM]);

      // Open the panels
      Servos[PANELACTION].write(myServos[PANELACTION][OPEN_POS], myServos[PANELACTION][OPEN_SPEED]);
      Serial.println("Panel opened to position " + String(myServos[PANELACTION][OPEN_POS]));

      delay(1500);
      Servos[PANELACTION].detach();
      Serial.println("Panels opened!");
      allPanelsOpen=true;
    }
    i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command
    digitalWrite(STATUS_LED, LOW);
}

// ------------------------------------------------------
// ToggleAllPanels - Toggles all panels from open-closed
// ------------------------------------------------------
void ToggleAllPanels() {
  Serial.println("Toggling all panels at the same time");

  if (allPanelsOpen) { // then close all panels
    for( int i = 0; i <  NUMBER_OF_SERVOS; i++) {
      Servos[i].attach(myServos[i][PIN_NUM]);
      Servos[i].write(myServos[i][CLOSED_POS],myServos[i][CLOSE_SPEED]);
      delay(1000);  // pause 1 second to allow for close to finish
      Servos[i].detach(); //Detach from the Servos to save power and cut potential jitter/hum.
    }
    allPanelsOpen = false;
  } else { // then open all panels
    for( int i = 0; i <  NUMBER_OF_SERVOS; i++) {
      Servos[i].attach(myServos[i][PIN_NUM]);
      Servos[i].write(myServos[i][OPEN_POS],myServos[i][OPEN_SPEED]);
      delay(1000);  // pause 1 second to allow for close to finish
      Servos[i].detach(); //Detach from the Servos to save power and cut potential jitter/hum.
    }
    allPanelsOpen = true;
  }
  i2cCommand = -1;  // always reset i2cCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);
}

// -----------------------------------------------------
// SweepPanels
// -----------------------------------------------------
void SweepPanels() {
    
  Serial.println("Panels sweep starting...");
  
  // attach to and then move each panel from CLOSED to OPEN to CLOSED
  // -- opening the panels
  for (int i = 0 ; i < NUMBER_OF_SERVOS ; i++) {
    Servos[i].attach(myServos[i][PIN_NUM]);
    Servos[i].write(myServos[i][OPEN_POS], myServos[i][OPEN_SPEED]);
  }
  delay(1500); // delay to allow movement to finish
  // -- close the panels
  for (int i = 0 ; i < NUMBER_OF_SERVOS ; i++) {
    Servos[i].attach(myServos[i][PIN_NUM]);
    Servos[i].write(myServos[i][CLOSED_POS], myServos[i][CLOSE_SPEED]);
  }
  delay(1500); // delay to allow movement to finish

  // detach from all panels to prevent jitter/burnout
  for (int i = 0 ; i < NUMBER_OF_SERVOS ; i ++) {
    Servos[i].detach();
  }

  Serial.println("Panel sweep complete!");
  i2cCommand = -1; // always reset i2cCommand to -1, so we don't repeatedly do the same command
}

// -----------------------------------------------------
// WavePanels - a rolling wave, counter-clockwise
// -----------------------------------------------------
void WavePanels() {

  Serial.println("Panel wave starting...");

  // begin opening a panel then move forward in a wave to close them behin the wave peak
  for (int i = 0 ; i < NUMBER_OF_SERVOS ; i++) {
    // begin opening the panels
    Servos[i].attach(myServos[i][PIN_NUM]);
    Servos[i].write(myServos[i][OPEN_POS], myServos[i][OPEN_SPEED]);
    delay(500);

    if (i >= 1) {
      // begin closing the trailing panel
      Servos[i-1].write(myServos[i][CLOSED_POS], myServos[i][CLOSE_SPEED]);
    }
  }

  // close the last panels since they aren't in the wave above
  Servos[NUMBER_OF_SERVOS-2].write(myServos[NUMBER_OF_SERVOS-2][CLOSED_POS], myServos[NUMBER_OF_SERVOS-2][CLOSE_SPEED]);
  delay(500);
  Servos[NUMBER_OF_SERVOS-1].write(myServos[NUMBER_OF_SERVOS-1][CLOSED_POS], myServos[NUMBER_OF_SERVOS-1][CLOSE_SPEED]);  
  delay(500); // delay to allow movement to finish

  // detach from all panels to prevent jitter/burnout
  for (int i = 0 ; i < NUMBER_OF_SERVOS ; i ++) {
    Servos[i].detach();
  }

  Serial.println("Panel wave complete!");
  i2cCommand = -1; // always reset i2cCommand to -1, so we don't repeatedly do the same command
}


// -----------------------------------------------------
// CloseAllPanels
// NOTE:  This assumes they are OPEN and will close them.  Will stress
//           the servos for those that are already closed.  This should be
//           called as an "emergency close" function only
// -----------------------------------------------------
void CloseAllPanels() {
  Serial.println("Emergency panel close starting...");
  
  // -- close the panels
  for (int i = 0 ; i < NUMBER_OF_SERVOS ; i++) {
    Servos[i].attach(myServos[i][PIN_NUM]);
    Servos[i].write(myServos[i][CLOSED_POS], myServos[i][CLOSE_SPEED]);
  }
  delay(1500); // delay to allow movement to finish

  // detach from all panels to prevent jitter/burnout
  for (int i = 0 ; i < NUMBER_OF_SERVOS ; i ++) {
    Servos[i].detach();
  }
  allPanelsOpen = false;

  Serial.println("Panel close complete!");
  i2cCommand = -1; // always reset i2cCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);
}


// --------------------------------------------------------------
// receivei2cEvent - processes all received I2C events
// --------------------------------------------------------------
void receivei2cEvent(int howMany) {
  if(howMany == 2)
  {
    i2cCommand  = Wire.read() << 8; 
    i2cCommand |= Wire.read();
    Serial.print("Received I2C value of: ");
    Serial.println(i2cCommand);
  }
  else
  {
    Serial.print("Unexpected number of bytes received: ");
    Serial.println(howMany);
  }
}
