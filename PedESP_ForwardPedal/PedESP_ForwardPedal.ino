/*
 * Lawton Jones
 * University of Vermont
 * Department of Mechanical Engineering 
 * CMPE 3815: Microcontrollers
123456789101112131415
  // Create test data
 
  // Generate a random integer
  int potVal = analogRead(PIN_POT);   //0-4095 
  
  // Format structured data
  strcpy(myData.a, "RED LED:");
  myData.b = potVal;
  
  // Send message via ESP-NOW


 * 14 Apr 2026
 * ESP-Now Communication
 * The purpose of this script is to trasnmit the input of a potentiometer value mapped to an LED PWM Brightness to another esp 
 *
 *
 * ------------------ HARDWARE -------------------
 * 1 ESP32
 * 1 logitech pedal
 * 
*/


/*
  iESP-NOW Demo - Transmit
  esp-now-demo-xmit.ino
  Sends data to Responder
  
  DroneBot Workshop 2022
  https://dronebotworkshop.com
  
*/
 
// Include Libraries
#include <esp_now.h>
#include <WiFi.h>
 
const byte PIN_GAS_POT = 32;    //right gas pedal potentiometer pin
const byte PIN_BRAKE_POT = 35;    //right gas pedal potentiometer pin
// Variables for test data
int LEDBrightness;


 
// MAC Address of responder - edit as required
uint8_t broadcastAddress[] = {0x3C, 0xDC, 0x75, 0x6E, 0x90, 0x24};   //mac address of the receiver (3)
 
// Define a data structure
typedef struct struct_message {
  char a[32];
  int b;
  int c;
} struct_message;
 
// Create a structured object
struct_message myData;
 
// Peer info
esp_now_peer_info_t peerInfo;
 
// Callback function called when data is sent
// Change "const uint8_t *mac_addr" to "const wifi_tx_info_t *info"
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
 
void setup() {
  
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
 
void loop() {
 
  // Create test data
 
  // Generate a random integer
  int gasVal = analogRead(PIN_GAS_POT);   //0-4095        //When pushed down the Pedal is analog reading ~490-550 when unpushed reading ~3300-3400
  int brakeVal = analogRead(PIN_BRAKE_POT);   //0-4095      //when pushed down the pedal is anlaog reading ~190-250 when unpushed reading ~2900-3100

  gasVal = constrain(map(gasVal,600,3200,255,0), 0, 255);   //take the inconsistent val from gas pedal to a full analog range
  brakeVal = constrain(map(brakeVal,300,2900,255,0), 0, 255);   //take the inconsistent val from gas pedal to a full analog range

  // Format structured data
  strcpy(myData.a, "PEDALS");
  myData.b = gasVal;
  myData.c = brakeVal;
  
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result != ESP_OK) {
    Serial.println("Sending error");
  }
  delay(10);
}