//https://copperhilltech.com/blog/esp32-serial-ports-uart0-uart1-uart2-access-using-the-arduino-ide/


#include <Arduino.h>
#include <HardwareSerial.h>
#include <esp_now.h>
#include <WiFi.h>


//----------------------------------------- INIT ------------------------------------------------------
//Assign UART1 and init UART1 info
HardwareSerial mySerial(1);

#define RxPIN         17
#define TxPIN         16
#define BAUDRATE      9600
#define SER_BUF_SIZE  1024



int steeringVal = 0;
int gasVal = 0;
int brakeVal = 0;


//--------------- Message Structure ---------
// Define a data structure **Must be consistent between all communicating ESPs
// a = ESP SENDING CODE
// b = GAS PEDAL VALUE
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

  //based on data recieved we set the gas, brake and steering values

  //DATA FROM PEDALS
  if(strcmp(myData.a, "PEDALS") == 0){
    gasVal = myData.b;     //GasPedal Value from the pedals. 
    brakeVal = myData.c;       //Brake Pedal value from pedals
    Serial.print("Data Received-- ");
    Serial.print("Gas: ");                
    Serial.print(gasVal);
    Serial.print(" | Brake: ");       //When pushed down the Pedal is analog reading ~490-550 when unpushed reading ~3300-3400
    Serial.println(brakeVal);
  }
  else if(strcmp(myData.a, "STEERING") == 0){
    /////PUT STEERING WHEEL DATA IN HERE
    
  }
}




//-------------------------------------- FUNCTION -----------------------------------------------






//----------------------------------------------- SETUP --------------------------------------------
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





//----------------------------------------- LOOP ------------------------------------------------
void loop() {
  //load variables to send to arduino
  int rPedal = gasVal;
  int lPedal = brakeVal;
  int steerAngle = steeringVal;

  //Send data in Packets for the arduino to interpret <rPedal,lPedal,steerAngle>
  mySerial.print("<");
  mySerial.print(rPedal);
  mySerial.print(",");
  mySerial.print(lPedal);
  mySerial.print(",");
  mySerial.print(steerAngle);
  mySerial.print(">");
 
  Serial.print("Data Sent");
  delay(50);    //50ms refresh rate for the data to send 
}
