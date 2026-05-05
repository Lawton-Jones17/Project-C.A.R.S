/*
 * -------------------------------------------------------------------------
 * PROJECT: Project C.A.R.S. | Wireless Rover Control
 * MODULE:  Steering Wheel Transmitter (IMU-based)
 * AUTHOR:  Lawton Jones
 * COLLEGE: University of Vermont | Dept. of Mechanical Engineering
 * COURSE:  CMPE 3815: Microcontrollers
 * DATE:    4 May 2026
 * -------------------------------------------------------------------------
 * * DESCRIPTION:
 * This script transforms a 2 breadboard steering wheel into a wireless control 
 * interface using an Inertial Measurement Unit (IMU). It calculates the 
 * wheel's rotational orientation through sensor fusion and broadcasts the 
 * steering angle to the Rover via the ESP-NOW protocol.
 * * OPERATION LOGIC:
 * 1. Sensor Fusion: Implements a Complementary Filter—combining 98% Gyroscope 
 * integration (for fast motion) and 2% Accelerometer data (for long-term 
 * stability)—to provide a smooth, drift-free angle.
 * 2. Normalization: Shifts the coordinate seam to the bottom (180° offset) 
 * and constrains the output to a -90° to 90° integer range.
 * 3. User Interfacing: Includes physical button interrupts for "Zeroing" 
 * (defining the center point) and "Calibration" (calculating Gyro bias).
 * 4. Async Transmission: Utilizes non-interfering millis() timing to send 
 * "STEER" data packets at 20Hz without disrupting the filter's dt integration.
 * * HARDWARE CONFIGURATION:
 * - MCU: ESP32 (Transmitter)
 * - Sensor: MPU9255 (9-Axis IMU) via I2C (SDA: 21, SCL: 22)
 * - Inputs: Reset/Zero Button (Pin 13), Calibration Button (Pin 12)
 * - Power: 5V Power Supply Module (common ground with ESP32)
 * - Communications: ESP-NOW (Peer-to-Peer Wireless)
 * -------------------------------------------------------------------------
 * SOURCES:
 * -  https://dronebotworkshop.com  -iESP-NOW Demo - Transmit
 *
 *
 */


//--------------------------------- LIBRARIES -------------------------------------
// Include Libraries
#include <esp_now.h>
#include <WiFi.h>
#include <MPU9255.h>//include MPU9255 library





//--------------------------------- INIT ----------------------------------------- 
MPU9255 mpu;   //init accelerometer



//define the acclerometer and I2C pins
#define PIN_RESET_BTN  13    //right button to reset the steering angle
#define PIN_CALIBRATE_BTN 12   //left button to calibrate the gyroscope on a flat surface.
#define SDA  21
#define SCL  22


//Intialize the variables for calculations Angle of steering wheel 
float zeroAngle = 0;        //var that holds current angle of system when zeroed
float filteredAngle = 0;    // var that holds angle of steady angle based on gyro damping 
float gyroBias = 0;         // Calibration offset 

// define variables for timed printing and data sending
unsigned long lastPrintTime = 0;
unsigned long lastTime = 0; 
unsigned long lastDelayPrintTime = 0;
 
// MAC Address of responder - mac address of the receiver
uint8_t broadcastAddress[] = {0x3C, 0xDC, 0x75, 0x6E, 0x90, 0x24};   //mac address of the receiver (3)
 
// Define a data structure **Must be consistent between all communicating ESPs
// char is the Transmitter Code to be recognized by receiver
// b is the steering angle value
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





//------------------------------ FUNCTIONS -------------------------------------- 

//---------- CALIBRATE GYRO -----------
// Function to calibrate the gyroscope when it is placed on flat ground
// takes the average of 500 gyro readings and assigns it to the global variable gyroBias
void calibrateGyro() {
  long sum = 0;
  for (int i = 0; i < 500; i++) {
    mpu.read_gyro();
    sum += mpu.gz;
    delay(2);
  }
  gyroBias = sum / 500.0; 
}








//--------------------------------- SETUP --------------------------------------- 
void setup() {
  // Set up Serial Monitor
  Serial.begin(115200);

  // begin I2C protocol on pins 16 and 17
  Wire.begin(SDA, SCL);

  //MPU initialization check
  if(mpu.init()){
    Serial.println("initialization Successful");
  } else{
    Serial.println("initialization Failed");
  }

  //set the scales and bandwidth for the Accelerometer and Gyroscope
  mpu.set_acc_scale(scale_2g);  //2g, 4g, 8g, 16g
  mpu.set_gyro_scale(scale_500dps);    //500 degrees/sec
  mpu.set_acc_bandwidth(acc_20Hz);
  mpu.set_gyro_bandwidth(gyro_20Hz); 

  //Set button pins to inputs with pullup resistors
  pinMode(PIN_RESET_BTN, INPUT_PULLUP);
  pinMode(PIN_CALIBRATE_BTN, INPUT_PULLUP);
 
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
 





//------------------------------------ LOOP ---------------------------------------
void loop() {

  // Timing for Integration
  unsigned long currentTime = millis();
  float dt = (currentTime - lastTime) / 1000.0;     // Change in time in seconds
  lastTime = currentTime;
  
  // read data from the MPU6500
  mpu.read_acc();      //get data from the accelerometer
  mpu.read_gyro();     //get data from the gyroscope

  // Assign raw data values from MPU to arrays of their X,Y,Z values
  float Accelerometer_values[3] = {mpu.ax, mpu.ay, mpu.az};
  int gyroscope_values[3] = {mpu.gx, mpu.gy, mpu.gz};

   //Convert the raw data to values of G
   for (byte I = 0; I < 3; I++){
     Accelerometer_values[I] = Accelerometer_values[I]/16384.0;   // The scale is 2G and vary between -32,764 - 32,764 meaning 1G in raw data = 16384
   }
  
  // Calc angle from MPU 0
  float accAngle = (atan2(Accelerometer_values[1], Accelerometer_values[0]) * 180 / PI) + 180;  // x and y values from the accelerometer. Then converts from radians to degrees.
                                                                                                // +180 moves the seam to the bottom of the wheel rather than having to handle negative logic in later code.
  if (accAngle > 180) accAngle -= 360; //Move the seam to the bottom of the steeringwheel so that math is easier
  

  // Get Gyro Rate (The "Fast" Motion)
  // Scale factor for MPU6500 at 500deg/s is 65.5
  float gyroRate = (mpu.gz - gyroBias) / 65.5;

  // Filter to keep steering smooth and remove accelerometer noise
  // New Angle = 98% of (Old Angle + Gyro change) + 2% of (Accelerometer Angle)
  filteredAngle = 0.98 * (filteredAngle + gyroRate * dt) + 0.02 * (accAngle);


  //------------------ Button Press Logic -------------------
  // Button Press Logic for reset button
  if (digitalRead(PIN_RESET_BTN) == LOW){
    zeroAngle = filteredAngle;  // zeroed angle is set to current angle
    delay(20);                  //debounce delay
  }

   // Button Press Logic for reset button
  if (digitalRead(PIN_CALIBRATE_BTN) == LOW){
    calibrateGyro();    // gyro bias is reset in order to recalibrate on flat surface
    delay(20);          //debounce delay
  }

  //calculate the angle from the zeroed angle
  float correctAngle = filteredAngle - zeroAngle;
  int steerVal = (int)constrain(correctAngle, -90, 90); // Hard limit for steering angle (-90, 90) degree range


  // Format structured data
  strcpy(myData.a, "STEER");     // Message for recevier to know which data it is receiving 
  myData.b = steerVal;           // Steering angle for the rover to interpret (-90,90) range
  myData.c = 0;       // Sending no c Data
  

  // Send message via ESP-NOW every 50 ms non interfering (To prevent issues with the gyroscope filter integration)
  if (millis() - lastDelayPrintTime > 50) {  //only send every 50ms
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    if (result != ESP_OK) {
      Serial.println("Sending error");
    }
    lastDelayPrintTime = millis(); //reset last time of data sent
  }

  //----- Print For Testing (Non-interfering)
  if (millis() - lastPrintTime > 100) { // Only print every 100ms
    //print 3 steering values for debugging
    Serial.print("Acc: "); Serial.print(accAngle);
    Serial.print(" | Filtered: "); Serial.print(correctAngle);
    Serial.print(" | Steer: "); Serial.println(steerVal);
    lastPrintTime = millis();  //reset last time of print
  }
}




//---------------------------------- END OF SKETCH -------------------------------------
//---------------------------------- END OF SKETCH -------------------------------------
//---------------------------------- END OF SKETCH -------------------------------------