#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLE2901.h>
#include <Preferences.h>


// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID                    "d98e357f-3d21-4669-a17d-9b389d6559e1"
#define CHARACTERISTIC_UUID_BTN_UP      "019f2af2-6401-445b-a52d-8119aca2c5ef"
#define CHARACTERISTIC_UUID_BTN_DOWN    "4e9ca473-b618-4de5-a0db-bb1c055a5e1c"
#define CHARACTERISTIC_UUID_DEVICE_NAME "019b018d-9736-7719-bf76-b09e14816156"
#define DEFAULT_DEVICE_NAME             "Maker's Pedals"
#define PREFS_NAMESPACE                 "pedals"
#define PREFS_DEVICE_NAME               "device-name"

// Button pins (fixed syntax - removed semicolons)
#define SWITCH_PIN_0 4
#define SWITCH_PIN_1 6
#define SWITCH_PIN_2 7

#define BUTTON_UP HIGH
#define BUTTON_DOWN LOW

Preferences preferences;
bool deviceConnected = false;

// Server callbacks to track connection status
class BluePedalCallbacks: public BLEServerCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String newDeviceName = pCharacteristic->getValue();
        setDeviceName(newDeviceName);
    }

    void onRead(BLECharacteristic *pCharacteristic) {
        String currentDeviceName = getDeviceName();
        pCharacteristic->setValue(currentDeviceName.c_str());
        Serial.println("Read Request for Device Name: " + currentDeviceName);
    }

    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("BLE Client connected");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("BLE Client disconnected");
      // Restart advertising
      pServer->getAdvertising()->start();
    }
  public:
    String getDeviceName() {
      preferences.begin(PREFS_NAMESPACE, true);
      String deviceName = preferences.getString(PREFS_DEVICE_NAME, String(DEFAULT_DEVICE_NAME));
      preferences.end();
      return deviceName;
    }

    String setDeviceName(String deviceName){
      preferences.begin(PREFS_NAMESPACE, false);
      preferences.putString(PREFS_DEVICE_NAME, deviceName);
      preferences.end();
      return deviceName;
    }
};


int buttonStatus0 = HIGH;
int buttonStatus1 = HIGH;
int buttonStatus2 = HIGH;

// Debouncing variables
unsigned long lastDebounceTime0 = 0;
unsigned long lastDebounceTime1 = 0;
unsigned long lastDebounceTime2 = 0;
unsigned long debounceDelay = 50;    // 50ms debounce delay

BLECharacteristic *pCharacteristicDown;
BLECharacteristic *pCharacteristicUp;
BLECharacteristic *pCharacteristicDeviceName;
BluePedalCallbacks *bluePedalCallbacks;

void firstTime(){

}

void setup() {
  // Set up serial communication for debugging
  Serial.begin(19200);
  delay(2000); // wait for serial monitor to connect after reset
  Serial.println("BLE Server initialization...");
  
  // Initialize button pins as inputs with pull-up resistors
  pinMode(SWITCH_PIN_0, INPUT_PULLUP);
  pinMode(SWITCH_PIN_1, INPUT_PULLUP);
  pinMode(SWITCH_PIN_2, INPUT_PULLUP);

  bluePedalCallbacks = new BluePedalCallbacks();
  
 
   // Read initial button states
  buttonStatus0 = digitalRead(SWITCH_PIN_0);
  buttonStatus1 = digitalRead(SWITCH_PIN_1);
  buttonStatus2 = digitalRead(SWITCH_PIN_2);

  // Initialize BLE with device name
  BLEDevice::init(bluePedalCallbacks->getDeviceName());
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BluePedalCallbacks());
  Serial.println("BLE initialized, creating Characteristics");

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create characteristics with notify capability for sending button updates
  pCharacteristicDown = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_BTN_DOWN,
                                           BLECharacteristic::PROPERTY_NOTIFY
  
                                       );
  Serial.println("Characteristic buttonDown created");

  pCharacteristicUp = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_BTN_UP,
                                          BLECharacteristic::PROPERTY_NOTIFY   
                                       );
  Serial.println("Characteristic buttonUp created");

//  pCharacteristicDown->addDescriptor(new BLE2902());
// Adds also the Characteristic User Description - 0x2901 descriptor
//  BLE2901 *descriptorDown_2901; 
//  descriptorDown_2901 = new BLE2901();
//  descriptorDown_2901->setDescription("Button Down Events");
//  descriptorDown_2901->setAccessPermissions(ESP_GATT_PERM_READ);  // enforce read only - default is Read|Write
//  pCharacteristicDown->addDescriptor(descriptorDown_2901);
//  Serial.println("Characteristic buttonDown descriptord created");

//  pCharacteristicUp->addDescriptor(new BLE2902());
//  BLE2901 *descriptorUp_2901; 
  // Adds also the Characteristic User Description - 0x2901 descriptor
//  descriptorUp_2901 = new BLE2901();
//  descriptorUp_2901->setDescription("Button Up Events");
//  descriptorUp_2901->setAccessPermissions(ESP_GATT_PERM_READ);  // enforce read only - default is Read|Write
//  pCharacteristicUp->addDescriptor(descriptorUp_2901);
//  Serial.println("Characteristic buttonUp descriptord created");


  pCharacteristicDeviceName = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_DEVICE_NAME,
                                          BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
                                       );
  Serial.println("Characteristic deviceName created");

  Serial.println("Starting Service");

  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE Server started and advertising...");
  Serial.printf("Monitoring buttons on pins %i, %i, and %i\n", SWITCH_PIN_0, SWITCH_PIN_1, SWITCH_PIN_2);
  preferences.begin(PREFS_NAMESPACE, true);

  Serial.printf("Device Name: %s\n",bluePedalCallbacks->getDeviceName().c_str());
}

void notifyButtonChanged(int buttonIndex, int buttonCurrentStatus) {
  // Map pin numbers to button names for clearer output
  int pinNumber;
  switch(buttonIndex) {
    case 0: pinNumber = SWITCH_PIN_0; break;
    case 1: pinNumber = SWITCH_PIN_1; break;
    case 2: pinNumber = SWITCH_PIN_2; break;
    default: pinNumber = -1; break;
  }
  
  // Create status message
  String statusMessage = "Button " + String(buttonIndex) + " (Pin " + String(pinNumber) + ") ";
  String bleMessage = "BTN" + String(buttonIndex) + ":";
  
  if (buttonCurrentStatus == BUTTON_DOWN) {
    statusMessage += "PRESSED";
    bleMessage += "1";  // Button pressed (LOW due to pull-up)
  } else {
    statusMessage += "RELEASED";
    bleMessage += "0";  // Button released (HIGH due to pull-up)
  }
  
  // Always print to Serial
  Serial.println(statusMessage);
  
  // Send notification via BLE only if client is connected
  if (deviceConnected) {
    if(buttonCurrentStatus == BUTTON_DOWN){
      pCharacteristicDown->setValue((uint8_t *)&buttonIndex, 4);
      pCharacteristicDown->notify();
    }
    if(buttonCurrentStatus == BUTTON_UP){
      pCharacteristicUp->setValue((uint8_t *)&buttonIndex, 4);
      pCharacteristicUp->notify();
    }
  }
}



void loop() {
  unsigned long currentTime = millis();
  
  // Read button states
  int buttonCurrentStatus0 = digitalRead(SWITCH_PIN_0);
  int buttonCurrentStatus1 = digitalRead(SWITCH_PIN_1);
  int buttonCurrentStatus2 = digitalRead(SWITCH_PIN_2);
  
  // Check button 0 with debouncing
  if (buttonCurrentStatus0 != buttonStatus0) {
    if ((currentTime - lastDebounceTime0) > debounceDelay) {
      notifyButtonChanged(0, buttonCurrentStatus0);
      buttonStatus0 = buttonCurrentStatus0;
      lastDebounceTime0 = currentTime;
    }
  }
  
  // Check button 1 with debouncing
  if (buttonCurrentStatus1 != buttonStatus1) {
    if ((currentTime - lastDebounceTime1) > debounceDelay) {
      notifyButtonChanged(1, buttonCurrentStatus1);
      buttonStatus1 = buttonCurrentStatus1;
      lastDebounceTime1 = currentTime;
    }
  }
  
  // Check button 2 with debouncing
  if (buttonCurrentStatus2 != buttonStatus2) {
    if ((currentTime - lastDebounceTime2) > debounceDelay) {
      notifyButtonChanged(2, buttonCurrentStatus2);
      buttonStatus2 = buttonCurrentStatus2;
      lastDebounceTime2 = currentTime;
    }
  }
  
  // Small delay to prevent excessive polling
  delay(10);
}
