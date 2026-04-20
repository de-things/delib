// [tested on ESP32-C5]

#include <DethingsDevice.h>

DethingsDevice device(
  "ESP32C5LOL",         // name
  "your_ssid",          // ssid
  "your_key"            // key
);

void setup() {
  device.init(); // call delib init
}

void loop() {
  if (device.getCommand() == "!whoami") {  // we'll invoke this command in [example.js]
    device.sendResponse(device.getName()); // to return device name as a response.
  }

  device.update(); // call delib update loop
}