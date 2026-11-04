#include "DeviceStorage.h"

#ifndef DeviceSetup_H
#define DeviceSetup_H

class DeviceSetup {
public:
    /**
    * Begins (enters) setup GUI.
    */
    esp_err_t begin(const char* hostname, const char* ssid, const char* password) {
        String _hostname = "";
        String _ssid = "";
        String _password = "";

        esp_err_t err {};
        
        // clear serial input buffer
        while(Serial.available() > 0) {
            Serial.read();
        }

        Serial.println();
        Serial.println("🔧 CONFIG MENU");
        Serial.println("⚠️ Tip: Send '*' to keep unchanged");
        Serial.println("⚠️ Tip: Send '/rm' to remove value");
        Serial.println();
        
        readItem("🛜 WiFi Hostname", hostname, _hostname);
        readItem("🛜 WiFi SSID", ssid, _ssid);
        readItem("🛜 WiFi Password", strlen(password) == 0 ? "" : "****", _password);
        readPrivateKeyAnswer("🙈 Re-gen private key? [y / *]");


        if (_hostname == "/rm") {
            err = device_storage.nvsRemove("name");
            if (err) return err;
        }
        else if (_hostname != "*") {
            err = device_storage.nvsWrite("name", _hostname.c_str());
            if (err) return err;
        }

        if (_ssid == "/rm") {
            err = device_storage.nvsRemove("ssid");
            if (err) return err;
        }
        else if (_ssid != "*") {
            err = device_storage.nvsWrite("ssid", _ssid.c_str());
            if (err) return err;
        }

        if (_password == "/rm") {
            err = device_storage.nvsRemove("pwd");
            if (err) return err;
        }
        else if (_password != "*") {
            err = device_storage.nvsWrite("pwd", _password.c_str());
            if (err) return err;
        }
        
        Serial.println("🟡 Saving changes...");
        Serial.println();

        // `true` if config written and config mode state is set to 0, 
        // `false` otherwise
        return device_storage.nvsWrite("config", "0");
    }

    /**
     * Returns `true` if device is in config mode according to NVS data.
     */
    bool isConfigMode() {
        std::vector<char> out {};
        if (!device_storage.nvsRead("config", out)) {
            return String(out.data()) == "1" ? true : false;
        }
        return false;
    }

    /**
     * Checks if any serial available, then reads it
     * and if it equals to `/config` sets config state to 1
     * + reboots the device.
     */
    void listen() {
        if (Serial.available() > 0) {
            String str = "";
            while (Serial.available() > 0) {
                char c = Serial.read();
                if (c == '\n' || c == '\r' || c == ' ') continue;
                str += c;
            }

            str.toLowerCase();

            // try to enable config mode trigger
            if (str == "/config") {
                Serial.print("🟡 Rebooting in configuration mode...");
                Serial.println();
                esp_err_t err = device_storage.nvsWrite("config", "1");

                if (!err) ESP.restart();
                else Serial.print("🔴 Cannot enter configuration mode, ERR CODE: "); Serial.println(err);
            }
            // try to clear nvs
            if (str == "/erase_all") {
                Serial.println("🟠 Erasing all data [5s]...");
                Serial.println("⚠️ Tip: Reboot to cancel");
                Serial.println();
                delay(5000);

                esp_err_t err = device_storage.nvsClear();

                if (!err) ESP.restart();
                else Serial.print("🔴 Cannot erase all data, ERR CODE: "); Serial.println(err);
            }
        }
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