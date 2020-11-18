#include <Arduino.h>
#include <WiFi.h>
#include <NimBLEDevice.h>

#define SERVICE_UUID "06a81d22-1524-11eb-adc1-0242ac120002" // UART service UUID
#define CHARACTERISTIC_UUID_TX "06a82060-1524-11eb-adc1-0242ac120002"

#define CHARACTERISTIC_TIMESTAMP "06a81f66-1524-11eb-adc1-0242ac120002"
#define CHARACTERISTIC_TOKEN "06a82132-1524-11eb-adc1-0242ac120002"
#define CHARACTERISTIC_ID_COBOX "06a821fa-1524-11eb-adc1-0242ac120002"

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
int incomingByte = 0;

void sentNotification( const char* test )
{
  const char *data =test ;

  char buffer[20] = {0};
  
  size_t len = strlen(data);        // doesn't count terminator
  size_t blen = sizeof(buffer) - 1; // doesn't count terminator
  size_t i = 0;
  
  for (i = 0; i < len / blen; ++i)
  {
    memcpy(buffer, data + (i * blen), blen);
    puts(buffer);
    pTxCharacteristic->setValue(buffer);
    pTxCharacteristic->notify();
  }

  if (len % blen)
  {
    puts(data + (len - len % blen));
    String stre = ((data + (len - len % blen)));
    // stre.replace("\0","");
    pTxCharacteristic->setValue(stre);
    pTxCharacteristic->notify();
  }
}


class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
  }
};
class TimeStamp : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    String rxValue = "";
    rxValue = rxValue + "timestamp : "+ pCharacteristic->getValue().c_str();
    Serial.println(rxValue);
    Sender.println(rxValue);
  }
};
class TokenCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    String rxValue = "";
    rxValue = rxValue + "token : "+ pCharacteristic->getValue().c_str();
    Serial.println(rxValue);
    Sender.println(rxValue);
  }
};
class IdCoboxCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    String rxValue = "";
    rxValue = rxValue + "coboxID : "+ pCharacteristic->getValue().c_str();
    Serial.println(rxValue);
    Sender.println(rxValue);
    // sentNotification();
  }
};


void connectedTask(void *parameter)
{
  for (;;)
  {
    
    if (deviceConnected)
    {
      if (Sender.available() > 0)
      {
        String str = Sender.readStringUntil('\n');
        const char *test = str.c_str();
        sentNotification(test);       
      }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected)
    {
      pServer->startAdvertising(); // restart advertising
      printf("start advertising\n");
      oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected)
    {
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // Delay between loops to reset watchdog timer
  }
  vTaskDelete(NULL);
}

void setupBLE()
{
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX,
      NIMBLE_PROPERTY::NOTIFY);

  BLECharacteristic *TimeStampCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_TIMESTAMP,
      NIMBLE_PROPERTY::WRITE);
  TimeStampCharacteristic->setCallbacks(new TimeStamp());

  BLECharacteristic *TokenCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_TOKEN,
      NIMBLE_PROPERTY::WRITE);
  TokenCharacteristic->setCallbacks(new TokenCallbacks());

  BLECharacteristic *IdCoboxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_ID_COBOX,
      NIMBLE_PROPERTY::WRITE);
  IdCoboxCharacteristic->setCallbacks(new IdCoboxCallbacks());

  // Start the service
  pService->start();

  xTaskCreate(connectedTask, "connectedTask", 5000, NULL, 1, NULL);
  // Start advertising

  pServer->getAdvertising()->start();
  printf("Waiting a client connection to notify...\n");
}

extern "C" void app_main()
{
  BLEDevice::init("UART Service");
  setupBLE();
  initArduino();
}