#define BLYNK_DEBUG // Optional, this enables lots of prints
#define BLYNK_PRINT Serial

// DHT Temperature & Humidity Sensor
// Unified Sensor Library Example
// Written by Tony DiCola for Adafruit Industries
// Released under an MIT license.

// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <BlynkSimpleWiFiNINA.h> // for Blynk
#include <Arduino_MKRENV.h> // For the ENV Shield
//
//// Blynk credentials
char auth[] = "123";
char ssid[] = "123";
char pass[] = "123";

//MKR board pin location:-
//AREF DAC0 A1 A2 A3 A4 A5 A6 0 1 2 3 4 5
//5V VIN VCC G RST TX RX SCL SDA MISO SCK MOSI
#define DehumidifierRelayPin 1
#define Fan1RelayPin 3
#define Heater2RelayPin 4
#define Heater1RelayPin 5
#define DHTPIN 7 // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment the type of sensor in use:
//#define DHTTYPE DHT11 // DHT 11
#define DHTTYPE DHT22 // DHT 22 (AM2302)
//#define DHTTYPE DHT21 // DHT 21 (AM2301)

// See guide for details on sensor wiring and usage:
// https://learn.adafruit.com/dht/overview

//define Blynk virtual pins
#define vTempSetPoint V0
#define vHeaterActive V1
#define vFanActive V2
#define vDehumActive V3
#define vSystemOnOff V4
#define vEnvironmentalTemp V5
#define vEnvironmentalHumidity V6
#define vTerminal V7
#define vDehumidifierSetPoint V8
#define vSystemStatus V9
#define vVariableStatus V10
#define vSystemActive V11


// define variables & set defaults
bool isFirstConnect = true; // Keep this flag not to re-sync on every reconnection
bool bSystemOn = false;
bool bHeaterActive = false;
bool bFanActive = false;
bool bDehumActive = false;
bool bDisplayStatus = false;
bool bDisplayVariable = false;
int iTempSetPoint = 25;
int iHeaterOnTemp = 23; // CHANGE ME back to 23
int iHeaterOffTemp = 27;
int iFanOnTemp = 27;
int iFanOffTemp = 20;
int iDehumidifierSetpoint = 50;
int iReadFailCount = 0;
float EnvironmentTemp;
float EnvironmentHumidity;


void controlHeatersAndFans(){

   // if env temp is <= iHeaterOffTemp variable & the heater flag is low switch Heater_Relay pin(s)"on"
  if (EnvironmentTemp <= iHeaterOffTemp && !bHeaterActive){
    digitalWrite(Heater1RelayPin, HIGH);
    // digitalWrite(Heater2RelayPin, HIGH);
    bHeaterActive = true; // toggle the heater flag
    Serial.print("Turning on heater1\n");
  }
  
   //if env temp is <= iTempSetPoint variable & the fan flag is high switch Heater2_Relay pin(s)"on"
  //Bring on the second heater if struggling to get to temp
  if (EnvironmentTemp <= iTempSetPoint){
    digitalWrite(Heater2RelayPin, HIGH);
    Serial.print("Turning on heater2\n");
  }
  
   //turn off the second heater once temp set point is reached
  if (EnvironmentTemp >= iTempSetPoint && bHeaterActive ){
    digitalWrite(Heater2RelayPin, LOW);
    Serial.print("Turning off heater2\n");
  }
  
   //if the env temp is >= to HeaterOff varibale & the heater flag is high then turn the Heater_Relay pin(s) off
  if (EnvironmentTemp >= iHeaterOffTemp && bHeaterActive ){
    digitalWrite(Heater1RelayPin, LOW);
    digitalWrite(Heater2RelayPin, LOW);
    bHeaterActive = false; // toggle the heater flag
    Serial.print("Turning off heater1\n");
    Serial.print("Turning off heater2\n");
  }
  
   // if env temp >= iFanOnTemp variable and fan is not running switch Fan_Relay pin(s) "on"
  if (EnvironmentTemp >= iFanOnTemp && !bFanActive){
    digitalWrite(Fan1RelayPin, HIGH);
    bFanActive = true;
    Serial.print("Turning on fan1\n");
  }
  // if env temp <= iFanOffTemp variable and fan is running then switch Fan_Relay pin(s) "Off"
  if (EnvironmentTemp <= iFanOffTemp && bFanActive){
    digitalWrite(Fan1RelayPin, LOW);
    bFanActive = false;
    Serial.print("Turning off fan1\n");
  }  
}



DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;

void setup() {
  Blynk.begin(auth, ssid, pass);
  Serial.begin(9600);
  // Initialize device.
  dht.begin();
  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print (F("Driver Ver: ")); Serial.println(sensor.version);
  Serial.print (F("Unique ID: ")); Serial.println(sensor.sensor_id);
  Serial.print (F("Max Value: ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print (F("Min Value: ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print (F("Resolution: ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print (F("Driver Ver: ")); Serial.println(sensor.version);
  Serial.print (F("Unique ID: ")); Serial.println(sensor.sensor_id);
  Serial.print (F("Max Value: ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print (F("Min Value: ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print (F("Resolution: ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;



  // set up digital pins
  pinMode(Heater1RelayPin, OUTPUT);// set mode
  digitalWrite(Heater1RelayPin, LOW);
}

void blynkWrite() {
  Blynk.virtualWrite(V5, EnvironmentTemp);
}

void loop() {
  // Delay between measurements.
  delay(delayMS);
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature!"));
    } else {
      Serial.print(F("Temperature: "));
      Serial.print(event.temperature);
      Serial.println(F("°C"));
      EnvironmentTemp = event.temperature;
    }
    
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  } else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
    EnvironmentHumidity = event.relative_humidity;
  }
  Blynk.run();
  blynkWrite();
  controlHeatersAndFans();

//  if (EnvironmentTemp <= iHeaterOnTemp) {
//    Serial.print("Heater On!!\n");
//    digitalWrite(Heater1RelayPin, HIGH);
//  } else {
//    Serial.print("Heater Off!\n");
//  }

//  Serial.print(EnvironmentTemp);
//  Serial.print("\n");
//  Serial.print(EnvironmentHumidity);
//  Serial.print("\n");
}





