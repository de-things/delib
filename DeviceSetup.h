#include "DeviceStorage.h"

#ifndef DeviceSetup_H
#define DeviceSetup_H

class DeviceSetup {
public:
    /**
    * Initializes (enters) setup GUI.
    */
    bool init(const char* ssid, const char* password) {
        String _ssid = "";
        String _password = "";
        
        bool result = false;

        // clear serial input buffer
        while(Serial.available() > 0) {
            Serial.read();
        }
        
        Serial.println("# Configuration setup");
        Serial.println("Tip: Send '*' to keep unchanged");

        readItem("> WiFi SSID", ssid, _ssid);
        readItem("> WiFi password", password, _password);

        readPrivateKeyAnswer("> Recreate private key? [y / *]");

        if (_ssid != "*") {
            if (!device_storage.nvsWrite("ssid", _ssid.c_str())) {
                result = true;
            }
        }
        if (_password != "*") {
            if (!device_storage.nvsWrite("pass", _password.c_str())) {
                result = true;
            }
        }
        
        Serial.println("# Configuration updated");
        Serial.println();

        return result;
    }

private:
    // Device storage object to init credentials data
    DeviceStorage device_storage {};

    /**
     * Removes private key data from the nvs if answer was positive.
     */
    void readPrivateKeyAnswer(String q) {
        String ans = "";
        Serial.println(q);
        while(Serial.available() == 0);
        while(Serial.available() > 0) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') continue;
            ans += c;
        }

        if (ans == "y" || ans == "Y") {
            device_storage.nvsRemove("pkey");
        }
    }

    void readItem(String title, const char* def_value, String& ref) {
        // print item line like [Example (def_value): ]
        Serial.print(title); Serial.print(" ("); Serial.print(strlen(def_value) == 0 ? "None" : def_value); Serial.println("):");
        // wait for input
        while(Serial.available() == 0);
        // clear old value
        ref = "";
        // get new item value
        while(Serial.available() > 0) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') continue;
            ref += c;
        }

        Serial.println();
    }
};

#endif