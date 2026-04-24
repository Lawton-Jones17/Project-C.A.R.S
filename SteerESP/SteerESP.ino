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
#include <MPU9255.h>//include MPU9255 library
 

MPU9255 mpu;

#define PIN_RESET_BTN  13    //right gas pedal potentiometer pin
#define SDA  21
#define SCL  22

float zeroAngle = 0; //var that holds current angle of system when zeroed
 
// MAC Address of responder - edit as required
uint8_t broadcastAddress[] = {0x14, 0x33, 0x5C, 0x47, 0xF8, 0x80};   //mac address of the receiver (3)
 
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
  //Serial.print("\r\nLast Packet Send Status:\t");
  //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
 
void setup() {
  // Set up Serial Monitor
  Serial.begin(115200);

  Wire.begin(SDA, SCL);

  //MPU initialization check
  if(mpu.init()){
    Serial.println("initialization Failed");
  } else{
    Serial.println("initialization successful!");
  }

  //set the scales and bandwidht for the Accelerometer and Gyroscope
  mpu.set_acc_scale(scale_2g);  //2g, 4g, 8g, 16g
  mpu.set_gyro_scale(scale_500dps);
  mpu.set_acc_bandwidth(acc_20Hz);
  mpu.set_gyro_bandwidth(gyro_20Hz); 


  pinMode(PIN_RESET_BTN, INPUT_PULLUP);
 
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
  
  //read data from the MPU6500
  mpu.read_acc();//get data from the accelerometer
  mpu.read_gyro();//get data from the gyroscope

  //Assign raw data values from MPU to arrays of their X,Y,Z values
  float Accelerometer_values[3] = {mpu.ax, mpu.ay, mpu.az};
  int gyroscope_values[3] = {mpu.gx, mpu.gy, mpu.gz};

  //Convert the raw data to values of G
  for (byte I = 0; I < 3; I++){
    Accelerometer_values[I] = Accelerometer_values[I]/16384.0;   // The scale is 2G and vary between -32,764 - 32,764 meaning 1G in raw data = 16384
  }
  
  //Calc angle from MPU 0
  float angle = atan2(Accelerometer_values[1], Accelerometer_values[0]) * 180 / PI;  // x and y values from the accelerometer. Then converts from radians to degrees

  //Button Press Logic
  if (digitalRead(PIN_RESET_BTN) == LOW){
    zeroAngle = angle;  // zeroed angle is set to current angle
    delay(20);          //debounce delay
  }

  //calculate the angle from the zeroed angle
  float correctAngle = angle - zeroAngle;
  int steerVal = int(correctAngle);

  // print the steering angle and the servo angle to the serial monitor
  Serial.print("Angle(deg): ");
  Serial.println(correctAngle);


  // Format structured data
  strcpy(myData.a, "STEER");
  myData.b = steerVal;
  myData.c = 0;       // Sending no c Data
  
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result != ESP_OK) {
    //Serial.println("Sending error");
  }
  delay(10);
}