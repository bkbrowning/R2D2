// -------------------------------------------------
// R2's Body Servo Expander
// Brent Browning
// -------------------------------------------------
// v 1.0 - Initial configuration and testing for panels
// v 1.1 - Modded for specific panel controls
// -------------------------------------------------

#include <VarSpeedServo.h> //Variable speed servo library
#include <Wire.h>  // i2c Library for communications

// -------------------------------------------------
// Define some constants to help reference objects, 
// pins, servos, leds etc by name instead of numbers
// -------------------------------------------------
#define STATUS_LED 13 // Our Status LED on Arduino Digital IO #13, this is the built in LED on the Pro Mini
#define DETACH 1


// -------------------------------------------------
// New outgoing i2c Commands
// -------------------------------------------------
String Packet;
int count = 0;
byte sum;
#define STEALTHI2C 0
#define BODYEXPANDERI2C 14
#define I2CTRANSMITTER 10

// These are easy names for our Arduino I/O pins and what they're used for 
// Note on the Servo expander board, numbering starts at 1,2,3 etc. 
// but internally we add 1 to get the real Arduino number.
#define RIGHT_FRONT_DOOR_SERVO_PIN 2
#define LEFT_FRONT_DOOR_SERVO_PIN 3
#define CHARGING_BAY_DOOR_SERVO_PIN 4
#define DATA_PANEL_DOOR_SERVO_PIN 5
#define RIGHT_REAR_DOOR_SERVO_PIN 6
#define LEFT_REAR_DOOR_SERVO_PIN 7

// Define and initialize the array to hold all of our servo data for servos connected TO THIS CONTROLLER
//    The array below is of the format...
//    {ArduinoPinNum, ServoClosedPosition, ServoOpenPosition, ServoCloseSpeed, ServoOpenSpeed}
#define NUMBER_OF_SERVOS 6 // total number of servos in the servo array below
VarSpeedServo Servos[NUMBER_OF_SERVOS];  // array to link all servos to the VarSpeedServo library
int myServos[NUMBER_OF_SERVOS][5] = {{ 4, 40, 175, 100, 100 },
                                     { 5, 40, 175, 100, 100 },
                                     { 2, 160, 40, 100, 100 },
                                     { 3, 175, 40, 100, 100 },
                                     { 6, 40, 140, 100, 100 },
                                     { 7, 175, 40, 100, 100 }};
// To make referring to our panels easier, we'll give them some more human-friendly names
// Note that the references below are to the servo array ROWS.  Should be one of these for each of NUMBER_OF_SERVOS
#define RF_DOOR 0      // PIN #2 ; Cable # 10 in my R2
#define LF_DOOR 1      // PIN #3 ; Cable # 5 in my R2
#define CHARGE_BAY 2   // PIN #4 ; Cable # 8 in my R2
#define DATA_PANEL 3   // PIN #5 ; Cable # 7 in my R2
#define RR_DOOR 4      // PIN #6 ; Cable # 9 in my R2
#define LR_DOOR 5      // PIN #7 ; Cable # 6 in my R2
// And we will define some constants that we can use to make referring to the servo array COLUMNS in the array above easier to remember
#define PIN_NUM 0
#define CLOSED_POS 1
#define OPEN_POS 2
#define CLOSE_SPEED 3
#define OPEN_SPEED 4

// Some variables to keep track of open/close status
boolean allDoorsOpen=false;
boolean doorStatus[NUMBER_OF_SERVOS] = {false, false, false, false, false};  // (false=closed; open=true)

int i2cCommand = 0;
int myi2c = BODYEXPANDERI2C; // the i2c address of this device

// ------------------------------------------
void setup() {
  Serial.begin(57600); // Debug Console
  Serial.println("R2 Body Servo Expander - v1.0");
  Serial.print("My i2C Address: ");
  Serial.println(myi2c);
  
  Wire.begin(myi2c);  // Start I2C communication Bus as a Slave (Device Number 9)
  Wire.onReceive(receivei2cEvent); // routine to call/run when we receive an i2c command
  pinMode(STATUS_LED, OUTPUT); // turn status led off
  digitalWrite(STATUS_LED, LOW);
  
  i2cCommand = 0;
  // Activate/Center all the servos to NEUTRAL position
  Serial.print("Activating Servos"); 
  resetServos();

  Serial.println("");  
}

//-----------------------------------------------------
// i2c Command Event Handler
//-----------------------------------------------------

void receivei2cEvent(int howMany) {
  i2cCommand = Wire.read();    // receive byte as an integer\
  Serial.print("Command Code:");
  Serial.println(i2cCommand); 
}

//----------------------------------------------------------------------------
// New i2c Commands
//----------------------------------------------------------------------------
void sendI2Ccmd(int deviceID, String cmd) {
  sum=0;
  Wire.beginTransmission(deviceID);
  for (int i=0;i<cmd.length();i++) {
    Wire.write(cmd[i]);
    sum+=byte(cmd[i]);
  }
  Wire.write(sum);
  Wire.endTransmission();  
}

//-----------------------------------------------------
// Reset all servos to starting positions
//-----------------------------------------------------
void resetServos() {
  for( int i = 0; i <  NUMBER_OF_SERVOS; i++) {
    Servos[i].attach(myServos[i][PIN_NUM]);
    Servos[i].write(myServos[i][CLOSED_POS],myServos[i][CLOSE_SPEED]);
  }

  delay(1000);  // pause 1 second to allow for close to finish
 
  // Detach from the servo to save power and reduce jitter
  if (DETACH) {
    for( int i = 0; i <  NUMBER_OF_SERVOS; i++) {
      Servos[i].detach(); //Detach from the Servos to save power and cut potential jitter/hum.
    }
  }

  allDoorsOpen=false;
  // update the door status array to hold the current state of the doors (false=closed; open=true)
  for (int i=0 ; i<NUMBER_OF_SERVOS; i++){
    doorStatus[i]=false;
  }
}



//-----------------------------------------------------
// D O O R S
//-----------------------------------------------------
void CycleAllDoors() {
  digitalWrite(STATUS_LED, HIGH);

  // If the doors are open the close them
  if (allDoorsOpen) { 
    Serial.println("Close doors");
    for( int i = 0; i <  NUMBER_OF_SERVOS; i++) {
      Servos[i].attach(myServos[i][PIN_NUM]);
      Servos[i].write(myServos[i][CLOSED_POS],myServos[i][CLOSE_SPEED]);
      delay(1000);  // pause 1 second to allow for close to finish
      if (DETACH) {
        Servos[i].detach(); //Detach from the Servos to save power and cut potential jitter/hum.
      }
    }
    allDoorsOpen=false; 

  } else {
    Serial.println("Open doors");
    for( int i = 0; i <  NUMBER_OF_SERVOS; i++) {
      Servos[i].attach(myServos[i][PIN_NUM]);
      Servos[i].write(myServos[i][OPEN_POS],myServos[i][OPEN_SPEED]);
      delay(1000);  // pause 1 second to allow for close to finish
      if (DETACH) {
        Servos[i].detach(); //Detach from the Servos to save power and cut potential jitter/hum.
      }
    }
    allDoorsOpen=true; 
  } 

  i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command 
  digitalWrite(STATUS_LED, LOW);
}

//-----------------------------------------------------
// C Y C L E   S I N G L E   D O O R
//-----------------------------------------------------
void CycleSingleDoor(int doorID) {
  digitalWrite(STATUS_LED, HIGH); // flash the LED for visual indicator that we are sending command
  if (doorStatus[doorID]) { // door is OPEN...so close it
    Serial.println("Closing a single door");
    Servos[doorID].attach(myServos[doorID][PIN_NUM]);
    Servos[doorID].write(myServos[doorID][CLOSED_POS],myServos[doorID][CLOSE_SPEED]);
    delay(1000);
    if (DETACH){
      Servos[doorID].detach();
    }
    doorStatus[doorID]=false;  // door is now closed
  } else {
    Serial.println("Opening a single door");
    Servos[doorID].attach(myServos[doorID][PIN_NUM]);
    Servos[doorID].write(myServos[doorID][OPEN_POS],myServos[doorID][OPEN_SPEED]);
    delay(1000);
    if (DETACH){
      Servos[doorID].detach();
    }
    doorStatus[doorID]=true;  // door is now open
  }

  i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command 
  digitalWrite(STATUS_LED, LOW);
}

//-----------------------------------------------------
// Main Loop
//-----------------------------------------------------

void loop() {

  delay(50);

  // Check for new i2c command
  switch(i2cCommand) {
    case 1: // RESET
          Serial.println("Got reset message");
          resetServos();
          digitalWrite(STATUS_LED, HIGH);
          i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command  
          break;
          
    case 2:
          CycleAllDoors();
          break;

    case 3: // Cycle the DataPanel door
          CycleSingleDoor(DATA_PANEL);
          break;
         
    case 4: // Cycle the Charging Panel door
          CycleSingleDoor(CHARGE_BAY);
          break;

    case 5: // Cycle ALL doors in Body and Panel...use this as trigger to Dome as well
          CycleAllDoors();  // cycles all the doors in the body
          //delay(1000);
          //sendI2Ccmd(I2CTRANSMITTER, "13");
          break;

    case 6: // Cycle the Right Front Panel door
          CycleSingleDoor(RF_DOOR);
          break;

    case 7: // Cycle the Left Rear Panel door
          CycleSingleDoor(LR_DOOR);
          break;

    case 8: // Cycle the Left Rear Panel door
          CycleSingleDoor(LF_DOOR);
          break;

    default: // Catch All
    case 0: 
          digitalWrite(STATUS_LED, LOW);
          i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command     
          break;
  }
}
