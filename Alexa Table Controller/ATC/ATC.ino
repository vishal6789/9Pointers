/*
 * Example
 *
 * If you encounter any issues:
 * - check the readme.md at https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md
 * - ensure all dependent libraries are installed
 * - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#arduinoide
 * - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#dependencies
 * - open serial monitor and check whats happening
 * - check full user documentation at https://sinricpro.github.io/esp8266-esp32-sdk
 * - visit https://github.com/sinricpro/esp8266-esp32-sdk/issues and check for existing issues or open a new one
 */

 // Custom devices requires SinricPro ESP8266/ESP32 SDK 2.9.6 or later

// Uncomment the following line to enable serial debug output
//#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
  #define DEBUG_ESP_PORT Serial
  #define NODEBUG_WEBSOCKETS
  #define NDEBUG
#endif

#include <Arduino.h>
#ifdef ESP8266
  #include <ESP8266WiFi.h>
#endif
#ifdef ESP32
  #include <WiFi.h>
#endif      

#include <SinricPro.h>
#include "Table.h"
#include <Preferences.h>

#define APP_KEY    "7faa328c-653f-4aa9-a3b2-5991469523b4"
#define APP_SECRET "ccdd1f18-b401-4611-b6a2-df7c80276a78-c9642118-e91e-49ff-9b70-c378e39bf32d"
#define DEVICE_ID  "63e9b9405ec7d92a47097294"

#define SSID       "9Pointers"
#define PASS       "9Pointers@281"

#define BAUD_RATE  9600

Table &table = SinricPro[DEVICE_ID];
Preferences preferences;

/*************
 * Variables *
 ***********************************************
 * Global variables to store the device states *
 ***********************************************/
 
unsigned int maxHeight = 60;
unsigned int relHeight; //current height
unsigned int cmPerSec = 5; // height of table movement per second
int relayDown = 5;  //d1 - relay pin for up direction
int relayUp = 2;    //d4 - relay pin for down direction

// PercentageController
int globalPercentage;

// RangeController
std::map<String, int> globalRangeValues;

/*************
 * Callbacks *
 *************/

//function to control relay for movement
//dir for direction ; true for up and false for down ;
int movement(bool dir, int height)
{

  height = (height>0)?height:(0-height);
  int delayTime= height*1000/cmPerSec;
  Serial.printf("\nMove table by %d in direction %s", height, dir?"UP":"DOWN");
    
  if(dir)
  {
    digitalWrite(relayUp, LOW);
    Serial.printf("\nRelay Up - %d",delayTime);
    delay(delayTime);
    digitalWrite(relayUp, HIGH);
    relHeight += height;
  }

  else
  {
    digitalWrite(relayDown, LOW);
    Serial.printf("\nRelay Down - %d",delayTime);
    delay(delayTime);
    digitalWrite(relayDown, HIGH);
    relHeight -= height;  
  }

   Serial.printf("\nNew Table height %d", relHeight);
   //saving height in permanent memory
   preferences.putUInt("relHeight", relHeight);
   return 1;
}


// PercentageController
bool onSetPercentage(const String &deviceId, int &percentage) {
  Serial.printf("\n[Device: %s]: Percentage level set to %d\r\n", deviceId.c_str(), percentage);
  int reqHeight = maxHeight*percentage/100; //required height
  movement(reqHeight>relHeight,reqHeight-relHeight);
  
  globalPercentage = percentage;
  updateRangeValue("heightRange", relHeight);
  return true; // request handled properly
}

bool onAdjustPercentage(const String &deviceId, int &percentageDelta) {
  globalPercentage += percentageDelta; // calculate absolute power level
  Serial.printf("\n[Device: %s]: Percentage level changed about %i to %d\r\n", deviceId.c_str(), percentageDelta, globalPercentage);
  
  int reqHeight = maxHeight*globalPercentage/100; //required height
  movement(reqHeight>relHeight,reqHeight-relHeight);
  percentageDelta = globalPercentage; // return absolute power level
  updateRangeValue("heightRange", relHeight);
  return true;                   // request handled properly
}

// RangeController
bool onRangeValue(const String &deviceId, const String& instance, int &rangeValue) {
  Serial.printf("\n[Device: %s]: Value for \"%s\" changed to %d\r\n", deviceId.c_str(), instance.c_str(), rangeValue);
  globalRangeValues[instance] = rangeValue;
  globalPercentage = rangeValue*100/maxHeight;
  movement(relHeight<rangeValue,relHeight-rangeValue);
  updatePercentage(globalPercentage);
  return true;
}

bool onAdjustRangeValue(const String &deviceId, const String& instance, int &valueDelta) {
  globalRangeValues[instance] += valueDelta;
  Serial.printf("\n[Device: %s]: Value for \"%s\" changed about %d to %d\r\n", deviceId.c_str(), instance.c_str(), valueDelta, globalRangeValues[instance]);
  globalRangeValues[instance] = valueDelta;
  int reqHeight = relHeight + valueDelta;
  globalPercentage = reqHeight*100/maxHeight;
  movement(reqHeight>relHeight,reqHeight-relHeight);
  updatePercentage(globalPercentage);
  return true;
}

/**********
 * Events *
 *************************************************
 * Examples how to update the server status when *
 * you physically interact with your device or a *
 * sensor reading changes.                       *
 *************************************************/

// PercentageController
void updatePercentage(int percentage) {
  table.sendSetPercentageEvent(percentage);
}

// RangeController
void updateRangeValue(String instance, int value) {
  table.sendRangeValueEvent(instance, value);
}


/********* 
 * Setup *
 *********/

void setupSinricPro() {

  // PercentageController
  table.onSetPercentage(onSetPercentage);
  table.onAdjustPercentage(onAdjustPercentage);

  // RangeController
  table.onRangeValue("heightRange", onRangeValue);
  table.onAdjustRangeValue("heightRange", onAdjustRangeValue);


  SinricPro.onConnected([]{ Serial.printf("\n[SinricPro]: Connected\r\n"); });
  SinricPro.onDisconnected([]{ Serial.printf("\n[SinricPro]: Disconnected\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
};

void setupWiFi() {
  WiFi.begin(SSID, PASS);
  Serial.printf("\n[WiFi]: Connecting to %s", SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  Serial.printf("\nconnected\r\n");
}

void setup() {
  Serial.begin(BAUD_RATE);
  preferences.begin("my-app", false);
  //Fetching last height of table from memory
  relHeight = preferences.getUInt("relHeight", 0);

  //Setting up rellay
  pinMode(relayUp, OUTPUT);
  pinMode(relayDown, OUTPUT);
  digitalWrite(relayUp, HIGH);
  digitalWrite(relayDown, HIGH);
  
  setupWiFi();
  setupSinricPro();

  Serial.printf("\nStarting Height of Table : %d",relHeight);
  
}

/********
 * Loop *
 ********/

void loop() {
  SinricPro.handle();
  
}
