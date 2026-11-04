> [!IMPORTANT]
> This library aims at ESP32-C5 controller, but it should work with most of ESP32 family as well. In addition, this library was tested on ESP32-C3 with no problems.

## Summary
Library to deploy decentralized IoT for ESP32 boards.

## Usage

There're 2 ways to use this library for your IoT device.

### A recommended way

```cpp
// Arduino IDE example firmware
// [Tested on ESP32C5]

#include <DethingsDevice.h>

DethingsDevice device {};

void setup() {
    // Call this on top of everything in setup.
    // [ This method already includes 'Serial.begin();' ]
    device.init();
}

void loop() {
    // This code wait in loop until device receive a POST request
    // containing "!whoami" string:
    if (device.getCommand() == "!whoami") {
        device.sendResponse(device.getName()); // sends response with a device name (hostname)
    }

    // Call this in the end of loop
    device.update();
}
```

### How to connect to WiFi and stuff?

When device is connected to a serial port send there: `/config`. It will reboot a device in config mode asking you to clarify various of stuff, including WiFi credentials. All of this data stores at [NVS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/nvs_flash.html), so restarting your device retrieves all of clarified data from it. Follow serial output instructions to get into more.

### A "hardcoded" way

This one assumes you won't use `/config` at all so all data is *hardcoded*:

```cpp
#include <DethingsDevice.h>

DethingsDevice device(
  "ESP32LOL",                         // Device hostname (string identifier over the network)
  "your_ssid",                        // WiFi name
  "your_pass",                        // WiFi password
  "7a8edf100cd1feabd847f5f4a4905b12"  // 32-byte private key (secret).
);

// Tip for a private key (secret):
// DO NOT use an example key!
// Instead, generate your own key using 'openssl' cli tool:
// ---
// ~]$ openssl rand -hex 16             <- use this command
// ~]$ 6446b2ea26babf959745f7b3cdb864c7 <- your private key will be generated right after

void setup() {
  device.init();
}

void loop() {
  if (device.getCommand() == "!whoami") {
    device.sendResponse(device.getName()); // responds with 'ESP32LOL'
  }

  device.update();
}
```

This implementation does not require you to use `/config`, but there's a trick: if you won't specify some data, let's say, a WiFi password, it's value will be retrieved from NVS during boot, so you can update it dynamically using a config mode. Other values will be protected from being retrieved by NVS no matter what you set in there.

Also, implementation exist as a sort of backward compatibility for old firmware working on delib, so you don't need to update your code, delib will handle problems and fill empty fields required.

## Install
1. Check if **[esp32 by Espressif](https://github.com/espressif/arduino-esp32)** board extension is installed for Arduino IDE if you flash though it.
2. Clone this repo to `libraries` directory.

```
For Linux:
'/home/USERNAME/Arduino/libraries'

For Windows: 
'C:\Users\USERNAME\Documents\Arduino\libraries'

So clone should be '~/Arduino/libraries/delib'
```