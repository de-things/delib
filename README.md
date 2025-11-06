> [!IMPORTANT]
> This version of lib **doesn't** work with Ethernet standard, since `Troyka-WiFi` isn't designed this way.
> This branch especially written for `Troyka-WiFi` if will be ever needed, but **won't be maintained anymore**.

### Summary
Core network lib to communicate with server side of de:things devices.

### Install
Clone repo into local arduino librares folder. Lib file should be located like:
```
C:\Users\your_username\Documents\Arduino\libraries\delib\delib.h
```

### Example relay firmware source
```cpp
// firmware.ino
#include <delib.h>

#define RELAY_PIN 14

bool enabled = false;

byte mac[] = { 0x15, 0x15, 0x15, 0x15, 0x15, 0x15 };

Delib delib = Delib();

void on() {
  if (delib.auth_wlan_request()) {
    digitalWrite(RELAY_PIN, HIGH);
    enabled = true;
  }
}
void off() {
  if (delib.auth_wlan_request()) {
    digitalWrite(RELAY_PIN, LOW);
    enabled = false;
  }
}
void state() {
  if (enabled) {
    delib.auth_wlan_request("1"); // send "1" as 200 response
  }
  else {
    delib.auth_wlan_request("0"); // send "0" as 200 response
  }
}
void setup() {
  pinMode(RELAY_PIN, OUTPUT);

  delib.server.on("/on", on);
  delib.server.on("/off", off);
  delib.server.on("/state", state);

  delib.set_wifi_credentials("Keenetic-5244", "NKzyTZJN");
  delib.set_device_name("GUInea Pig");

  delib.init(mac);
}

void loop() {
  delib.update();
}
```

### Example relay scheme
![example-wifi-relay-scheme](https://github.com/user-attachments/assets/c34ca3f8-a9e1-4a15-97f0-e99f59af8ad3)

### How to deploy example relay firmware
You need to install ESP8266 board [deps](https://github.com/esp8266/Arduino) and select `Generic ESP8266 Module` as a board to flash after deps has been installed. If deps link is unavailable for some reason, just find a board expansion for Arduino IDE to flash ESP8266-based controllers.

### Hardware used in example relay scheme
* [Troyka WiFi](https://wiki.amperka.ru/%D0%BF%D1%80%D0%BE%D0%B4%D1%83%D0%BA%D1%82%D1%8B:troyka-wi-fi)
* [Mini-Relay](https://wiki.amperka.ru/%D0%BF%D1%80%D0%BE%D0%B4%D1%83%D0%BA%D1%82%D1%8B:troyka-mini-relay)
* [Arduino Uno](https://amperka.ru/product/arduino-uno)
* [Blue LCD 16x2 1601 i2c](https://iarduino.ru/shop/displays/display-lcd-character-1601-i2c-white-on-blue.html)

Next step is download `LiquidCrystal_I2C` lib from [here](https://iarduino.ru/lib/1473854197-3978.zip) or from other source if mine is unavailable... You need to unpack it to:
```
C:\Users\your_username\Documents\Arduino\libraries\
```

After steps above you most likely to flash your `Troyka-WiFi`. Compilier can throw an exception of missing `Ethernet.h` with no reason. If so, download this lib and install in local libraries directory as well. But don't do this if everything compiles and flashes normally, you can face the lib duplicate otherwise.
