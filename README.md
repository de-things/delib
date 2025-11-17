> [!IMPORTANT]
> Don't import both libs at once, since wlan lib compiles for esp8266 controller and eth one for general arduino uno with ethernet shield installed.
> It potentially can work together if some controller has both interfaces to communicate with wlan and ethernet, but I didn't test it.
> 
> This lib most likely written and tested for Troyka modules by [amperka.ru](https://amperka.ru/)

### Summary
Core network lib for mirco contollers to initialize and handle their server side.

### Install
Clone repo into local arduino librares folder. Lib file should be located like:
```
C:\Users\your_username\Documents\Arduino\libraries\delib\*.*
```
