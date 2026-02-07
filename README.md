> [!CAUTION]
> This is test branch with ESP32 support.

## Summary
Core network lib for mirco contollers to initialize and handle their server side.

## Install
1. Clone repo to local arduino libraries directory. For Windows it's:
```
C:\Users\your_username\Documents\Arduino\libraries
```
2. Copy **[add-ons](https://github.com/de-things/delib/tree/main/add-ons)** to the local arduino libraries directory;
3. *(Optionally)* Install **[esp8266 by ESP8266 Community](https://github.com/esp8266/Arduino)** board extension if you plan to work with esp8266 controller [module](https://github.com/de-things/delib/blob/main/Delib8266.h).
4. *(Optionally)* Install **[esp32 by Espressif](https://github.com/espressif/arduino-esp32)** board extension if you plan to work with esp32 controller [module](https://github.com/de-things/delib/blob/main/Delib32.h).

## Examples

### ESP32
```cpp
#include <Delib32.h>

// ddv mac
byte mac[6] = { 0x00, 0x00, 0x00, 0x00, 0xff, 0x04 };

// delib instance
Delib32 delib = Delib32();

void setup() {
  delib.set_wifi_credentials("your_ssid", "your_password");

  // i will reconfigure it later for 128x64 i2c lcd screen, since it's more compact.
  delib.set_lcd_attributes(0x3F /* alternative: 0x27 */, 16, 2); // lcd screen parameters (based on 1602 lcd 16x2 screen)

  // init device
  delib.init(mac);
}

void loop() {
  if (delib.get_command() == "!ping") { 
    delib.send_response("pong");
  }

  // invoke delib update
  delib.update();
}
```

### ESP8266 / Ethernet
Check [main](https://github.com/de-things/delib/tree/main) branch readme.
