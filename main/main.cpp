#include <Arduino.h>
#include <NimBLEDevice.h>


extern "C" void app_main()
{
  BLEDevice::init("UART Service");
}