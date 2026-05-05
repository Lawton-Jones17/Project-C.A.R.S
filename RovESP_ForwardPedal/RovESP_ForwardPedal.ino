/*
 * -------------------------------------------------------------------------
 * PROJECT: Project C.A.R.S. | Wireless Rover Control
 * MODULE:  Rover Receiver & Serial Bridge
 * AUTHOR:  Lawton Jones
 * COLLEGE: University of Vermont | Dept. of Mechanical Engineering
 * COURSE:  CMPE 3815: Microcontrollers
 * DATE:    4 May 2026
 * -------------------------------------------------------------------------
 * * DESCRIPTION:
 * This script acts as the central communication hub for the Rover. It 
 * receives wireless control packets from multiple transmitters via ESP-NOW, 
 * aggregates the inputs (Gas, Brake, and Steering), and relays them to 
 * the primary motor control unit (Arduino) via a packetized UART stream.
 * * OPERATION LOGIC:
 * 1. Wireless Gateway: Listens for ESP-NOW traffic and utilizes a callback 
 * function to differentiate between "PEDALS" and "STEER" data sources.
 * 2. Data Synchronization: Stores the most recent 8-bit pedal values and 
 * steering angles into global variables for asynchronous processing.
 * 3. Serial Framing: Encapsulates the control data into a delimited packet 
 * format: "<rPedal,lPedal,steerAngle>" to ensure data integrity.
 * 4. Hardware Bridge: Transmits the formatted string over HardwareSerial1 
 * (UART1) at 9600 Baud to the downstream microcontroller every 50ms.
 * * HARDWARE CONFIGURATION:
 * - MCU: ESP32 (Receiver)
 * - Communication: UART1 (HardwareSerial) @ 9600 Baud
 * - Power: 7.4V Battery Pack (common ground with Arduino and ESP32)
 * - Pin 17: RX (Receive from Arduino)
 * - Pin 16: TX (Transmit to Arduino)
 * - Protocol: ESP-NOW (Wi-Fi Station Mode)
 * -------------------------------------------------------------------------
 *  SOURCES:
 * - https://copperhilltech.com/blog/esp32-serial-ports-uart0-uart1-uart2-access-using-the-arduino-ide/
 */




//---------------------------------- LIBRARIES -------------------------------------
#include <Arduino.h>
#include <HardwareSerial.h>
#include <esp_now.h>
#include <WiFi.h>




//------------------------------------ INIT ---------------------------------------
//Assign UART1 and init UART1 info (ESP has multiple Hardware Serials)
HardwareSerial mySerial(1);


//define serial data and pins
#define RxPIN         17
#define TxPIN         16
#define BAUDRATE      9600
#define SER_BUF_SIZE  1024



//init values that rover will receive for driving control
int steerVal = 0;
int gasVal = 0;
int brakeVal = 0;


//--------------- Message Structure ------------
// Define a data structure **Must be consistent between all communicating ESPs
// a = ESP SENDING CODE
// b = GAS PEDAL VALUE / STEERING VALUE
// c = BRAKE PEDAL VALUE
typedef struct struct_message {
  char a[32];
  int b;
  int c;
} struct_message;

// Create a structured object
struct_message myData;
 

// Callback function executed when data is received
void OnDataRecv(const esp_now_recv_info_t * recv_info, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));

  //based on data recieved we set the gas, brake and steering values to be sent to the arduino

  //DATA FROM PEDALS
  if(strcmp(myData.a, "PEDALS") == 0){     // Check for message from pedal transmitter
    gasVal = myData.b;         // GasPedal Value from the pedals (0,255) range
    brakeVal = myData.c;       // Brake Pedal value from pedals (0,255) range
    // //serial print to monitor values received for debugging
    // Serial.print("Data Received-- ");
    // Serial.print("Gas: ");                
    // Serial.print(gasVal);
    // Serial.print(" | Brake: ");       
    // Serial.println(brakeVal);
  }
  //DATA FROM STEERING
  else if(strcmp(myData.a, "STEER") == 0){     //check for message from steering transmitter
    steerVal = myData.b;      //Steering Value from Steering unit (-90, 90) range
    // //serial print steering value to monitor for debugging
    // Serial.print("Steer: ");                
    // Serial.println(steerVal);
    
  }
}






//----------------------------------- SETUP --------------------------------------
void setup() {
  //Setup Serial monitor and Serial Communication
  Serial.begin(115200);   //begin normal Serial Monitor at baud rate similar to all esp32s

  mySerial.setRxBufferSize(SER_BUF_SIZE);   //standard arduino has 64 bytes
                                            // ESP32 has 256 bytes
                                            //Call must come before begin()
  //Initialize the Serial Port (UART1) -- for communication with Arduino
  mySerial.begin(BAUDRATE, SERIAL_8N1, RxPIN, TxPIN);
  

  // Set ESP32 as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
 
  // Initilize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register callback function
  esp_now_register_recv_cb(OnDataRecv);


}





//------------------------------------ LOOP ---------------------------------------
void loop() {
  //load variables to send to arduino
  int rPedal = gasVal;          // value 0-255, 255 is fully pressed
  int lPedal = brakeVal;        // value 0-255, 255 is fully pressed
  int steerAngle = steerVal;    // -90 to 90 for steering wheel. 90 deg ccw turn is -90, 90 deg cw turn is 90 

  //Send data through serial hardware in Packets for the arduino to interpret <rPedal,lPedal,steerAngle>
  mySerial.print("<");
  mySerial.print(rPedal);
  mySerial.print(",");
  mySerial.print(lPedal);
  mySerial.print(",");
  mySerial.print(steerAngle);
  mySerial.print(">");
 
  //Serial.print("Data Sent");
  delay(50);    //50ms refresh rate for the data to send 
}



//---------------------------------- END OF SKETCH -------------------------------------
//---------------------------------- END OF SKETCH -------------------------------------
//---------------------------------- END OF SKETCH -------------------------------------