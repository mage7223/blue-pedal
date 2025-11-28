#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLE2901.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID              "d98e357f-3d21-4669-a17d-9b389d6559e1"
#define CHARACTERISTIC_UUID_BTN_UP  "019f2af2-6401-445b-a52d-8119aca2c5ef"
#define CHARACTERISTIC_UUID_BTN_DOWN "4e9ca473-b618-4de5-a0db-bb1c055a5e1c"

// Button pins (fixed syntax - removed semicolons)
#define SWITCH_PIN_0 8
#define SWITCH_PIN_1 9
#define SWITCH_PIN_2 10

#define BUTTON_UP HIGH
#define BUTTON_DOWN LOW

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
BLE2901 *descriptor_2901 = NULL;

bool deviceConnected = false;

// Server callbacks to track connection status
class MyServerCallbacks: public BLEServerCallbacks {
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
};


void setup() {
  Serial.begin(115200);
  Serial.println("BLE Server initialization...");
  
  // Initialize button pins as inputs with pull-up resistors
  pinMode(SWITCH_PIN_0, INPUT_PULLUP);
  pinMode(SWITCH_PIN_1, INPUT_PULLUP);
  pinMode(SWITCH_PIN_2, INPUT_PULLUP);
  
  // Read initial button states
  buttonStatus0 = digitalRead(SWITCH_PIN_0);
  buttonStatus1 = digitalRead(SWITCH_PIN_1);
  buttonStatus2 = digitalRead(SWITCH_PIN_2);

  // Initialize BLE
  BLEDevice::init("Sophie's Pedals");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  Serial.println("BLE initialized, creating Characteristics");

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create characteristic with notify capability for sending button updates
  pCharacteristicDown = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_BTN_DOWN,
                                          BLECharacteristic::PROPERTY_READ | 
                                          BLECharacteristic::PROPERTY_WRITE | 
                                          BLECharacteristic::PROPERTY_NOTIFY | 
                                          BLECharacteristic::PROPERTY_INDICATE
                                       );
  Serial.println("Characteristic buttonDown created");
  // Create characteristic with notify capability for sending button updates
  pCharacteristicUp = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_BTN_UP,
                                          BLECharacteristic::PROPERTY_READ | 
                                          BLECharacteristic::PROPERTY_WRITE | 
                                          BLECharacteristic::PROPERTY_NOTIFY | 
                                          BLECharacteristic::PROPERTY_INDICATE
                                       );

  Serial.println("Characteristic buttonUp created");

  pCharacteristicDown->addDescriptor(new BLE2902());
  // Adds also the Characteristic User Description - 0x2901 descriptor
  descriptor_2901 = new BLE2901();
  descriptor_2901->setDescription("Button Down Events");
  descriptor_2901->setAccessPermissions(ESP_GATT_PERM_READ);  // enforce read only - default is Read|Write
  pCharacteristicDown->addDescriptor(descriptor_2901);

  pCharacteristicUp->addDescriptor(new BLE2902());
  // Adds also the Characteristic User Description - 0x2901 descriptor
  descriptor_2901 = new BLE2901();
  descriptor_2901->setDescription("Button Up Events");
  descriptor_2901->setAccessPermissions(ESP_GATT_PERM_READ);  // enforce read only - default is Read|Write
  pCharacteristicUp->addDescriptor(descriptor_2901);

  pService->addCharacteristic(pCharacteristicDown);
  pService->addCharacteristic(pCharacteristicUp);
  Serial.println("Starting Service");

  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  // Add characteristics to advertising
  pAdvertising->setScanResponse(false);
  // Set preferred connection parameters for better compatibility with iOS devices
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

//  pAdvertising->start();
  
  Serial.println("BLE Server started and advertising...");
  Serial.printf("Monitoring buttons on pins %i, %i, and %i", SWITCH_PIN_0, SWITCH_PIN_1, SWITCH_PIN_2);
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