// [tested on ESP32-C5]

#include <DethingsDevice.h>

DethingsDevice device(
  "ESP32C5LOL",         // device hostname (custom string identifier over the network)
  "your_ssid",          // wifi name
  "your_key"            // wifi password
);

void setup() {
  device.init(); // call delib init
  
  /*
  Have to write in serial port (115200 baud)
  something like:
    ...............
    Device Name: ESP32C5LOL
    IP: 10.62.41.205
    MAC: AA:BB:CC:DD:EE:FF
  */
}

void loop() {
  if (device.getCommand() == "!whoami") {  // we'll invoke this command in [example.js]
    device.sendResponse(device.getName()); // to return device name as a response.
  }

  device.update(); // call delib update loop
}