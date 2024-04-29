#include <ArduinoBLE.h>
#include "Arduino_BMI270_BMM150.h"
#include "ClosedCube_MAX30205.h"  // Temperature sensor's lib
#include <SPI.h>
#include "protocentral_Max30003.h" // ECG 3 Click's lib

ClosedCube_MAX30205 max30205; // Temperature sensor
MAX30003 max30003; // ECG sensor
// create service:
BLEService tempService("00001809-0000-1000-8000-00805F9B34FB");

BLEService ecgService("123e4567-e89b-12d3-a456-426614174000");

// create characteristics and allow remote device to read and get notifications:
BLEFloatCharacteristic bodyTemp("00002A1C-0000-1000-8000-00805F9B34FB", BLERead | BLEIndicate);

BLEFloatCharacteristic ecgCharacteristic("123e4567-e89b-12d3-a456-426614174001", BLERead | BLENotify);

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize temp sensor
  max30205.begin(0x48);

  while (!BLE.begin()) {
    Serial.println("Waiting for BLE to start");
    delay(1);
  }

  // Initialize ECG sensor
  pinMode(MAX30003_CS_PIN,OUTPUT);
  digitalWrite(MAX30003_CS_PIN,HIGH); //disable device
  SPI.begin();
  SPI.beginTransaction(SPISettings(12000000,MSBFIRST,SPI_MODE0));

  bool ret = max30003.max30003ReadInfo();
  if(ret){
    Serial.println("Max30003 read ID Success");
  }else{
    while(!ret){
      //stay here untill the issue is fixed.
      ret = max30003.max30003ReadInfo();
      Serial.println("Failed to read ID, please make sure all the pins are connected");
      delay(10000);
    }
  }
  Serial.println("Initialising the chip ...");
  max30003.max30003Begin(); // Initialize MAX30003

  // set the local name that the peripheral advertises:
  BLE.setLocalName("BLE_Arduino");
  // set the UUID for the service:
  BLE.setAdvertisedService(tempService);
  BLE.setAdvertisedService(ecgService);

  // add the characteristics to the service
  tempService.addCharacteristic(bodyTemp);
  ecgService.addCharacteristic(ecgCharacteristic);
  // add the service
  BLE.addService(tempService);
  BLE.addService(ecgService);

  // start advertising the service:
  BLE.advertise();
}

void loop() {
  float temperature;
  float ecg;
  BLEDevice central = BLE.central();

  // if a central is connected to the peripheral:
  if (central) {
    // print the central's BT address:
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    // turn on LED to indicate connection:
    digitalWrite(LED_BUILTIN, HIGH);
    
    // while the central remains connected:
    while (central.connected()) {
      temperature = max30205.readTemperature(); // In Celsius
      Serial.print(F("Temp="));
      Serial.println(temperature, DEC);

      max30003.getEcgSamples();   //It reads the ecg sample and stores it to max30003.ecgdata
      ecg = max30003.ecgdata;
      Serial.print("ECG: ");
      Serial.println(ecg);
      
      // write sensor values to service characteristics:
      bodyTemp.writeValue(temperature);
      ecgCharacteristic.writeValue(ecg);

      delay(1000);
    }
  } else {
    // turn off the LED
    digitalWrite(LED_BUILTIN, LOW);
  }  
}