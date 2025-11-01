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

// include delib
#include <delib.h>

// define mac
uint8_t mac[] = { 13, 13, 13, 13, 13, 13 };

// define delib object
delib Delib;

void setup() {
  // set wifi credentials (ssid, key)
  Delib.set_wifi_credentials("whale_wifi", "wha1e_p@ss");

  // initialize delib server
  Delib.init(mac);
}

void loop() {
  // handle client<>server communication
  Delib.update();
}

```
