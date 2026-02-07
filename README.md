> [!IMPORTANT]
> It's highly recommended **not** to `#include` both `DelibWlan.h` and `DelibEthernet.h` modules at once, since the first one compiles for **Generic ESP8266 Controller** and the second one for most of **Arduino boards** with Ethernet interface installed.

## Summary
Core network lib for mirco contollers to initialize and handle their server side.

## Install
1. Clone repo to local arduino libraries directory. For Windows it's:
```
C:\Users\your_username\Documents\Arduino\libraries
```
2. Copy **[add-ons](https://github.com/de-things/delib/tree/main/add-ons)** to the local arduino libraries directory;
3. *(Optionally)* Install **[esp8266 by ESP8266 Community](https://github.com/esp8266/Arduino)** board extension if you plan to work with esp8266 controller [module](https://github.com/de-things/delib/blob/main/DelibWlan.h).

## Examples
Check **[delib-wifi-example](https://github.com/de-things/delib-wifi-example)** and **[delib-ethernet-example](https://github.com/de-things/delib-ethernet-example)** to get into how to use this lib.

## ESP32 Support?
~~I hate ESP32 server (no routing), so there is no ESP32 support.~~ **I'm working on it.**
