/*
 * -------------------------------------------------------------------------
 * PROJECT: Project C.A.R.S. | Wireless Rover Control
 * MODULE:  Pedal Controller Transmitter
 * AUTHOR:  Lawton Jones
 * COLLEGE: University of Vermont | Dept. of Mechanical Engineering
 * COURSE:  CMPE 3815: Microcontrollers
 * DATE:    4 May 2026
 * -------------------------------------------------------------------------
 * * DESCRIPTION:
 * This script serves as the primary control interface for the Rover system.
 * It reads analog voltage signals from two Logitech PS2 potentiometer pedals
 * (Gas and Brake), processes the raw 12-bit ADC data, and transmits the 
 * normalized values to the receiver ESP32 via the ESP-NOW protocol.
 * * OPERATION LOGIC:
 * 1. Data Acquisition: Reads analog inputs from PIN 32 (Gas) and 35 (Brake).
 * 2. Signal Scaling: Maps the specific physical range of the potentiometers 
 * (Gas: 600-3200 | Brake: 300-2900) to an 8-bit PWM-ready range (0-255).
 * 3. Encapsulation: Packages data into a 'struct_message' containing a 
 * transmitter ID ("PEDALS") and the two integer control values.
 * 4. Transmission: Broadcasts the packet via ESP-NOW at a 20Hz refresh rate 
 * (50ms delay) to ensure low-latency rover response.
 * * HARDWARE CONFIGURATION:
 * - MCU: ESP32 (Transmitter)
 * - Input: Logitech Pedal Assembly
 * - Power: 5V Power Supply Module (common ground with ESP32)
 * - Pin 32: Gas Potentiometer (Analog In)
 * - Pin 35: Brake Potentiometer (Analog In)
 * -------------------------------------------------------------------------
 */
 



//-------------------------------------- LIBRARIES -------------------------------------------- 
#include <esp_now.h>
#include <WiFi.h>



//----------------------------------------- INIT -------------------------------------------------- 
//define Gas and Brake pedal input pins
const byte PIN_GAS_POT = 32;    //right gas pedal potentiometer pin
const byte PIN_BRAKE_POT = 35;    //right gas pedal potentiometer pin


 
// MAC Address of responder - edit as required
uint8_t broadcastAddress[] = {0x3C, 0xDC, 0x75, 0x6E, 0x90, 0x24};   //mac address of the receiver (Rover ESP) (3)
 
//Define the Data structure that all ESP's recognize
// char is the Transmitter Code to be recognized by receiver
// b and c are gas and brake values
typedef struct struct_message {
  char a[32];
  int b;
  int c;
} struct_message;
 
// Create a structured object
struct_message myData;
 
// Peer info for ESP communication
esp_now_peer_info_t peerInfo;
 
// Callback function called when data is sent
// used to debug and comfirm ESP is properly sending data
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
 





//----------------------------------------- SETUP ------------------------------------------------------
void setup() {
  
  //init pedal pins
  pinMode(PIN_GAS_POT, INPUT);
  pinMode(PIN_BRAKE_POT, INPUT);

  // Set up Serial Monitor
  Serial.begin(115200);
 
  // Set ESP32 as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
 
  // Initilize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
 
  // Register the send callback
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}
 




//----------------------------------------- LOOP ------------------------------------------------------
void loop() {

 
  // Read the pedal positions 
  int gasVal = analogRead(PIN_GAS_POT);   //0-4095        //When pushed down the Pedal is analog reading ~490-550 when unpushed reading ~3300-3400
  int brakeVal = analogRead(PIN_BRAKE_POT);   //0-4095      //when pushed down the pedal is anlaog reading ~190-250 when unpushed reading ~2900-3100

  // Map Pedal values to 0-255 PWM value for Rover arduino
  gasVal = constrain(map(gasVal,600,3200,255,0), 0, 255);   //take the inconsistent val from gas pedal to a full analog range
  brakeVal = constrain(map(brakeVal,300,2900,255,0), 0, 255);   //take the inconsistent val from gas pedal to a full analog range


  // Format structured data
  strcpy(myData.a, "PEDALS");   // Transmitter Code to be understood by Receiver
  myData.b = gasVal;            // Gas Value (0-255)
  myData.c = brakeVal;          // Brake Value (0-255)
  
  // Send data message via ESP-NOW to receiver
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result != ESP_OK) {
    Serial.println("Sending error");
  }


  delay(50);      //Wait 50 ms so the data is sent at a rate that can be handled by ESP and Arudino
}




//----------------------------------------- END OF SKETCH ------------------------------------------------------
//----------------------------------------- END OF SKETCH ------------------------------------------------------
//----------------------------------------- END OF SKETCH ------------------------------------------------------
