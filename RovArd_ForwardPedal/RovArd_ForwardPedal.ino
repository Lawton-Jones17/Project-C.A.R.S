/*
 * -------------------------------------------------------------------------
 * PROJECT: Project C.A.R.S. | Wireless Rover Control
 * MODULE:  Rover Motor Actuation & Driving Logic
 * AUTHOR:  Lawton Jones
 * COLLEGE: University of Vermont | Dept. of Mechanical Engineering
 * COURSE:  CMPE 3815: Microcontrollers
 * DATE:    4 May 2026Motor
 * -------------------------------------------------------------------------
 * * DESCRIPTION:
 * This script serves as the primary power and movement controller for the 
 * rover. It decodes the telemetry packets received from the ESP32 serial 
 * bridge and translates those control signals into physical movement via 
 * an L293D motor driver shield and a four-motor drivetrain.
 * * OPERATION LOGIC:
 * 1. Packet Parsing: Listens on SoftwareSerial for the '<' start delimiter 
 * and parses the CSV-formatted gas, brake, and steering telemetry.
 * 2. Velocity Calculation: Derives a net 'speedVal' by subtracting brake 
 * input from gas input to determine forward/reverse magnitude.
 * 3. Kinematic Mapping: Implements a three-tier steering logic:
 * - Deadzone (±10°): Maintains synchronized RPM for straight travel.
 * - Differential Turn (10°-50°): Scales inner wheel speed relative to 
 * the steering angle for smooth arc turns.
 * - Pivot/Rotate (>50°): Executes zero-radius tank turns by reversing 
 * the rotation of inner wheels.
 * 4. Actuation: Interfaces with an 8-bit shift register to set H-bridge 
 * directions and applies PWM signals to regulate motor torque.
 * * HARDWARE CONFIGURATION:
 * - MCU: Arduino (Motor Controller)
 * - Motor Driver: L293D Driver Shield (Shift Register Interface)
 * - Comms: SoftwareSerial @ 9600 Baud (RX: A0 | TX: A1)
 * - PWM Pins: 3, 5, 6, 11 (Motor Speeds)
 * - Shift Register: Pins 4 (CLK), 7 (EN), 8 (DATA), 12 (LATCH)
 * -------------------------------------------------------------------------
 */





//------------------------------------ LIBRARIES ---------------------------------------
#include <Arduino.h>
#include <SoftwareSerial.h>




//--------------------------------------- INIT -----------------------------------------
//init Serial at analog pins
#define  RxPIN    A0
#define  TxPIN    A1

//Set up Software Serial Monitor for ESP communication with new pins
SoftwareSerial mySerial(RxPIN, TxPIN); // RX, TX 

// init variables that will be received for driving
int gasVal;
int brakeVal;
int steerAngle;



//------------------------------------ MOTOR INIT ---------------------------------------
//Pin defintions
const int PWM2A = 11;      //M1 motor PWM signal pin
const int PWM2B = 3;       //M2 motor PWM signal pin
const int PWM0A = 6;       //M3 motor PWM signal pin
const int PWM0B = 5;       //M4 motor PWM signal pin
const int DIR_CLK = 4;     // Data input clock line
const int DIR_EN = 7;      //Equip the L293D enabling pins
const int DATA = 8;        // USB cable
const int DIR_LATCH = 12;  // Output memory latch clock


//Binary Values of the motor directions in order to turn the motors the directions needed to turn to go the direction
const int Move_Forward = 39;       //Move Forward
const int Move_Backward = 216;     //Move Backward
const int Left_Move = 116;         //Left translation
const int Right_Move = 139;        //Right translation
const int Right_Rotate = 149;      //Rotate Right
const int Left_Rotate = 106;       //Rotate Left
const int Stop = 0;                //Stop
const int Upper_Left_Move = 36;    //Upper Left Move
const int Upper_Right_Move = 3;    //Upper Right Move
const int Lower_Left_Move = 80;    //Lower Left Move
const int Lower_Right_Move = 136;  //Lower Right Move
const int Drift_Left = 20;         //Drift on Left
const int Drift_Right = 10;        //Drift on Right

//Speed of motors using pwm
int Speed1 = 255;//Set the default speed between 1 and 255
int Speed2 = 255;//Set the default speed between 1 and 255
int Speed3 = 255;//Set the default speed between 1 and 255
int Speed4 = 255;//Set the default speed between 1 and 255
//----------------------------------------------




//---------------------------- MOTOR FUNCTION --------------------------------------------
//(input direction(binary val for shift reg), Motor Speeds 1,2,3,4)
void Motor(int Dir, int Speed1, int Speed2, int Speed3, int Speed4) {
  analogWrite(PWM2A, Speed1);  //Motor PWM speed regulation
  analogWrite(PWM2B, Speed2);  //Motor PWM speed regulation
  analogWrite(PWM0A, Speed3);  //Motor PWM speed regulation
  analogWrite(PWM0B, Speed4);  //Motor PWM speed regulation

  digitalWrite(DIR_LATCH, LOW);            //DIR_LATCH sets the low level and writes the direction of motion in preparation
  shiftOut(DATA, DIR_CLK, MSBFIRST, Dir);  //Write Dir motion direction value
  digitalWrite(DIR_LATCH, HIGH);           //DIR_LATCH sets the high level and outputs the direction of motion
}





//-------------------------------------- SETUP ---------------------------------------------
void setup() {
  Serial.begin(9600);    // begin serial monitor for computer communication
  mySerial.begin(9600);  // begin the serial communication between the ESP and Arduino (Software Serial)


  //Setup Pin Modes for Motor Shield all OUTPUT
  pinMode(DIR_CLK, OUTPUT);
  pinMode(DATA, OUTPUT);
  pinMode(DIR_EN, OUTPUT);
  pinMode(DIR_LATCH, OUTPUT);

  pinMode(PWM0B, OUTPUT);
  pinMode(PWM0A, OUTPUT);
  pinMode(PWM2A, OUTPUT);
  pinMode(PWM2B, OUTPUT);
}





//----------------------------------- LOOP ------------------------------------------------
void loop() {

//------------- pull data from esp --------------
  //if there is data in the serial monitor
  if (mySerial.available() > 0) {
    char startChar = mySerial.read();   //read serial data from esp

    // Check if this is the start of our data packet
    // if the data is formatted in the correct form read the driving control variables from esp
    if (startChar == '<') {
      gasVal = mySerial.parseInt(); // Reads up to the first comma, 
      brakeVal = mySerial.parseInt(); // Reads up to the second comma
      steerAngle  = mySerial.parseInt(); // Reads up to the '>'

      // Print the received data on the computer serial monitor for debugging
      //Serial.print("Gas: "); Serial.print(gasVal);
      //Serial.print(" | Brake: "); Serial.print(brakeVal);
      //Serial.print(" | Steer: "); Serial.println(steerAngle);
    }
  }



//------------------ DRIVING LOGIC ------------------------------
//------------ take data and actuate motor as needed -------------

  // Steup steering zone consts
  const int DEADZONE_THR = 10;
  const int SHARP_TURN_THR = 50;  
  
  // take difference of gas and brake pedal to get speed value: FWD(+), BWD(-)
  int speedVal = gasVal - brakeVal;
  //---- STOPPED----
  if (speedVal == 0){
    Motor(Stop, 0, 0, 0, 0);
  }

  //---- FORWARD ----
  else if (speedVal > 0){
    //---- STRAIGHT FWD ----
    // if steer anlge in the straight zone
    if(steerAngle <= DEADZONE_THR && steerAngle >= -DEADZONE_THR){       // Create deadzone range of angle of steering wheel. 10 deg in each direction
      Motor(Move_Forward, speedVal, speedVal, speedVal, speedVal);
    }
    //---- FWD RIGHT ----
    // if the steer angle in the slow right turn zone
    else if (steerAngle > DEADZONE_THR && steerAngle <= SHARP_TURN_THR){
      int outerSpeed = speedVal;
      int innerSpeed = map(steerAngle, DEADZONE_THR, SHARP_TURN_THR, 255, 0);  //Convert the steering angle to the inner wheel speed: 10deg small turn high speed, 60 deg sharp turn low speed
      Motor(Move_Forward, outerSpeed, innerSpeed, innerSpeed, outerSpeed);   //Motors 2,3 are the right motors(inner) and 1,4 are the left motors(outer)
    }
    //---- ROTATE RIGHT ----
    // if the steer angle in the sharp right turn zone
    else if (steerAngle > SHARP_TURN_THR){
      int outerSpeed = speedVal;
      int innerSpeed = map(steerAngle, SHARP_TURN_THR, 90, 0, 100);  //Convert the steering angle to the inner wheel speed: 60deg small turn no inner speed, 90deg sharp turn high BWD inner speed 
                                                                     //(Not Max it feels to aggressive 100 is sweet spot of not too powerful but still sharp enough)
      Motor(Right_Rotate, outerSpeed, innerSpeed, innerSpeed, outerSpeed);   //Motors 2,3 are the right motors(inner) and 1,4 are the left motors(outer)
    }
    // ---- FWD LEFT ----
    //if the steer angle in the slow left turn zone
    else if (steerAngle < -DEADZONE_THR && steerAngle >= -SHARP_TURN_THR){
      int outerSpeed = speedVal;
      int innerSpeed = map(steerAngle, -DEADZONE_THR, -SHARP_TURN_THR, 255, 0);  //Convert the steering angle to the inner wheel speed: 10deg small turn high speed, 90deg sharp turn low speed
      Motor(Move_Forward, innerSpeed, outerSpeed, outerSpeed, innerSpeed);   //Motors 2,3 are the right motors(outer) and 1,4 are the left motors(inner)
    }
    //---- ROTATE LEFT ----
    // if the steer angle in the sharp left turn zone
    else if (steerAngle < -SHARP_TURN_THR){
      int outerSpeed = speedVal;
      int innerSpeed = map(steerAngle, -SHARP_TURN_THR, -90, 0, 100);  //Convert the steering angle to the inner wheel speed: 60deg small turn no inner speed, 90deg sharp turn high BWD inner speed 
                                                                       //(Not Max it feels to aggressive 100 is sweet spot of not too powerful but still sharp enough)
      Motor(Left_Rotate, innerSpeed, outerSpeed, outerSpeed, innerSpeed);   //Motors 2,3 are the right motors(outer) and 1,4 are the left motors(inner)
    }
  }


  //---- BACKWARD ----
  // if the speed value is negative
  else if(speedVal < 0){
    int reverseVal = abs(speedVal);   // convert the negative speed val to positive to be handed to the motors correctly
    //---- STRAIGHT BWD----
    // if the steer angle in the straight zone
    if(steerAngle <= DEADZONE_THR && steerAngle >= -DEADZONE_THR){       // Create deadzone range of angle of steering wheel. 10 deg in each direction
      Motor(Move_Backward, reverseVal, reverseVal, reverseVal, reverseVal);
    }
    //(Backwards turning only has slower turn modes to make driving easier)
    //---- BWD RIGHT ----
    // if the steer angle in the right trun zone 
    else if (steerAngle > DEADZONE_THR){
      int outerSpeed = reverseVal;
      int innerSpeed = map(steerAngle, DEADZONE_THR, 90, 255, 0);  //Convert the steering angle to the inner wheel speed: 10deg small turn high speed, 90 deg sharp turn low speed
      Motor(Move_Backward, outerSpeed, innerSpeed, innerSpeed, outerSpeed);   //Motors 2,3 are the right motors(inner) and 1,4 are the left motors(outer)
    }
    //---- BWD LEFT ----
    // if the steer angle in the left turn zone.
    else if (steerAngle < -DEADZONE_THR){
      int outerSpeed = reverseVal;
      int innerSpeed = map(steerAngle, -DEADZONE_THR, -90, 255, 0);  //Convert the steering angle to the inner wheel speed: 10deg small turn high speed, 90 deg sharp turn low speed
      Motor(Move_Backward, innerSpeed, outerSpeed, outerSpeed, innerSpeed);   //Motors 2,3 are the right motors(outer) and 1,4 are the left motors(inner)
    }
  }
      
  
}



//------------------------------------ END OF SKETCH ---------------------------------------
//------------------------------------ END OF SKETCH ---------------------------------------
//------------------------------------ END OF SKETCH ---------------------------------------

