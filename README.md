> [!CAUTION]
> Docs is WIP.

### Summary
Core network lib to communicate with server side of de:things devices.

### Install
Clone repo into local arduino librares folder. Lib file should be located like:
```
C:\Users\your_username\Documents\Arduino\libraries\delib\delib.h
```

### Usage
```cpp
// firmware.ino

// include lib
#include <delib.h>

// set mac
uint8_t mac[] = { 13, 13, 13, 13, 13, 13 };

// lib instance
Delib delib;

// test handler for wlan connection
void testHandler() {
  if (delib.auth_wlan_request()) { // do something if client sent a proper secret
    // your code logic goes here
    // ...
    Serial.println("test passed!");
  }
}

void setup() {
  // set wlan secret
  delib.set_secret("SUPER_SECRET");

  // set wlan credentials
  delib.set_wifi_credentials("whale_ssid", "whale_password");

  // set wlan handler
  delib.wlan_server.on("/test", testHandler);

  // start profile select and servers initialization
  delib.init(mac);
}

void loop() {
  // test handler for ethernet connection
  if (delib.ethn_buffer == "test") { // do something if client sent "test"
    // your code logic goes here
    // ...
    Serial.println("test passed!");
  }

  // call server update
  delib.update();
}

```
