#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID              "55072829-bc9e-4c53-938a-74a6d4c78776"
#define CHARACTERISTIC_UUID_READ  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_WRITE "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Button pins (fixed syntax - removed semicolons)
#define SWITCH_PIN_0 3
#define SWITCH_PIN_1 10
#define SWITCH_PIN_2 9

int buttonStatus0 = LOW;
int buttonStatus1 = LOW;
int buttonStatus2 = LOW;

// Debouncing variables
unsigned long lastDebounceTime0 = 0;
unsigned long lastDebounceTime1 = 0;
unsigned long lastDebounceTime2 = 0;
unsigned long debounceDelay = 50;    // 50ms debounce delay

BLECharacteristic *pCharacteristic;
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

// Characteristic callbacks for incoming data
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue();

      if (value.length() > 0) {
        Serial.println("*********");
        Serial.print("New value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};

void setup() {
  Serial.begin(115200);
  
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

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create characteristic with notify capability for sending button updates
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_WRITE,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Button Monitor Ready");
  
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();
  
  Serial.println("BLE Server started and advertising...");
  Serial.println("Monitoring buttons on pins 3, 9, and 10");
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
  
  if (buttonCurrentStatus == LOW) {
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
    pCharacteristic->setValue(bleMessage.c_str());
    pCharacteristic->notify();
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