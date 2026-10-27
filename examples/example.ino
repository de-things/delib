// [tested on ESP32-C5]

#include <DethingsDevice.h>

DethingsDevice device(
  "ESP32C5LOL",                       // device hostname (string identifier over the network)
  "your_ssid",                        // wifi name
  "your_pass",                        // wifi password
  "7a8edf100cd1feabd847f5f4a4905b12"  // 32-byte private key (secret).
);

// Tip for a private key (secret):
// DO NOT use example secret!
// Instead, generate your own key using 'openssl' cli tool:
// ---
// ~]$ openssl rand -hex 16             <- use this command
// ~]$ 6446b2ea26babf959745f7b3cdb864c7 <- your private key will be generated right after

void setup() {
  device.init(); // call delib init
  
  // Approximate serial output after initialization:
  /*
    ...............
    Device name: ESP32C5LOL
    IP: 192.168.0.141
    MAC: AA:BB:CC:DD:EE:FF
  */
}

void loop() {
  if (device.getCommand() == "!whoami") {  // wait in loop for a '!whoami' command
    device.sendResponse(device.getName()); // send response as a device hostname.
  }

  device.update(); // call delib update
}