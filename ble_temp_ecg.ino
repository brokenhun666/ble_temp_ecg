#include <ArduinoBLE.h>
#include "ClosedCube_MAX30205.h"  // Temperature sensor's lib
#include <SPI.h>
#include "protocentral_Max30003.h" // ECG 3 Click's lib

#define POWER_LED 6
#define CONNECTION_LED 5

ClosedCube_MAX30205 max30205; // Temperature sensor
MAX30003 max30003; // ECG sensor

BLEDevice central;
// create service:
BLEService tempService("00001809-0000-1000-8000-00805f9b34fb");

BLEService ecgService("0000180d-0000-1000-8000-00805f9b34fb");

// create characteristics and allow remote device to read and get notifications:
BLEFloatCharacteristic bodyTemp("00002a1c-0000-1000-8000-00805f9b34fb", BLERead | BLEIndicate);

BLEFloatCharacteristic ecgCharacteristic("00002a37-0000-1000-8000-00805f9b34fb", BLERead | BLENotify);

float temperature;
float lastTemperature = 0;
float ecg;

void setup() {
  Serial.begin(9600);
  pinMode(POWER_LED, OUTPUT);
  pinMode(CONNECTION_LED, OUTPUT);
  digitalWrite(POWER_LED, HIGH);

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
  //BLE.setAdvertisedService(tempService);
  //BLE.setAdvertisedService(ecgService);

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
  central = BLE.central();
  // if a central is connected to the peripheral:
  if (central) {
    // print the central's BT address:
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    // turn on LED to indicate connection:
    digitalWrite(CONNECTION_LED, HIGH);
    
    // while the central remains connected:
    while (central.connected()) {
      temperature = max30205.readTemperature(); // In Celsius
    
      //Serial.print(F("Temp="));
      //Serial.println(temperature, DEC);

      max30003.getEcgSamples();   //It reads the ecg sample and stores it to max30003.ecgdata
      ecg = max30003.ecgdata;
      //Serial.print("ECG: ");
      Serial.println(ecg);
      
      // write sensor values to service characteristics:
      if(abs(temperature - lastTemperature) >= 0.1 || lastTemperature == 0) {
        lastTemperature = temperature;
        bodyTemp.writeValue(temperature);
      }
      ecgCharacteristic.writeValue(ecg);

      delay(8);
    }
  } else {
    // turn off the LED
    digitalWrite(CONNECTION_LED, LOW);
  }  
}
