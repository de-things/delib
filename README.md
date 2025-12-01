> [!IMPORTANT]
> It's highly recommended **not** to `#include` both `DelibWlan.h` and `DelibEthernet.h` modules at once, since the first one compiles for Generic ESP8266 Controller and the second one for Arduino Board with Ethernet interface installed.
> 
> Modules potentially can work together if some controller has both interfaces to communicate with WLAN and Ethernet (like based on ESP32 WT32-ETH01 controller), but I didn't test it and most likely won't to.
> 
> This lib written and tested for products by **[amperka.ru](https://amperka.ru/)** and **[iarduino.ru](https://iarduino.ru/)**.

## 🐈Summary
Core network lib for mirco contollers to initialize and handle their server side.

## 🐈Pre-requirements
1. Get **[ESP8266 Community](https://github.com/esp8266/Arduino)** board extenstion if you plan to work with WLAN module. `DelibWlan.h` designed for esp8266 controller;
2. Copy dependencies from **[add-ons](https://github.com/de-things/delib/tree/main/add-ons)** to the local arduino libraries directory.
3. Delib relies on `LiquidCrystal_I2C.h`, so make sure your logs screen supports it.

## 🐈Install
Clone repo into the local arduino librares folder. For Windows it should be:

```
~\Documents\Arduino\libraries\
```

## 🐈Examples
Check **[delib-wifi-example](https://github.com/de-things/delib-wifi-example)** and **[delib-ethernet-example](https://github.com/de-things/delib-ethernet-example)** to get into how to use this lib.

## 🐈Why no ESP32 support
I don't have this kind of controllers in my lab... But if I'll get them someday, I'll expand this core! xd
