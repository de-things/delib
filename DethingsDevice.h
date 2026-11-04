#include <WiFi.h>
#include <string.h>
#include <vector>
#include <mbedtls/gcm.h>
#include <mbedtls/base64.h>
#include <nvs.h>
#include <nvs_flash.h>
#include "DeviceSetup.h"
#include "DeviceStorage.h"
#include "DeviceCrypto.h"

/**
* Core network lib class to handle server side of de:things devices based on ESP32 family.
*/
class DethingsDevice {
public:
    /**
     * Dethings device instance.
     * `hostname` - device preferable hostname;
     * `ssid` - WiFi name to connect to (nvs value by default);
     * `password` - WiFi password to connect with (nvs value by default);
     * `pkey` - 32-byte private key (secret) used in AES-256-GCM encryption (nvs value by default).
     */
    DethingsDevice(const char* hostname = "", const char* ssid = "", const char* password = "", const char* pkey = "") {
        Serial.begin(115200);
        while (!Serial);

        Serial.println();
        Serial.println("🟢 Serial initialized");

        if (strlen(hostname) != 0) {
            _hostname = hostname;
        }
        if (strlen(ssid) != 0) {
            _ssid = ssid;
        }
        if (strlen(password) != 0) {
            _password = password;
        }
        if (strlen(pkey) != 0) {
            _pkey = pkey;
        }
    }

    /**
    * Initialization method. Use it to start a proper server after pre-requirements have finished.
    */
    void init() {
        esp_log_level_set("wifi", ESP_LOG_NONE);

        device_storage.init();

        // fetch nvs data
        fetchWiFiData(_hostname, _ssid, _password);

        // enter config mode if enabled
        if (device_setup.isConfigMode()) {
            esp_err_t err = device_setup.begin(_hostname, _ssid, _password);
            if (!err) {
                Serial.println("🟡 Rebooting...");
                Serial.println();
                ESP.restart(); // restart device (will start in normal mode)
            }
            else {
                Serial.print("🔴 Something went wrong with a config setup, ERR CODE: "); Serial.println(err);
                eternalSleep();
            }
        }

        if (strlen(_hostname) != 0)
            WiFi.setHostname(_hostname);
        
        // WiFi.setAutoReconnect(true);

        // this happens only if private key was defined in class constructor.
        if (strlen(_pkey) > 0 && !device_crypto.isPrivateKeyValid(String(_pkey))) {
            Serial.println("🔴 Hard-coded private key is invalid. If you unsure, remove private key from the constructor and let device generate it automatically on startup.");
            eternalSleep();
        }

        // establish connection to a specified wlan
        wifiBegin();

        // fetch private key
        // this method should be called only after wifi has been established
        // because of this: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/random.html
        if (WiFi.isConnected()) {
            fetchPrivateKey(_pkey);
        }

        // show state message after wlan initialization
        showStateMessage();
    }
    /**
    * Main loop of `delib`. Handles client<>server communication if established.
    */
    void update() {
        // check if serial input == "/config"
        device_setup.listen();

        // wlan client handling
        client = server.accept();
        
        if (client) {
            if (client.connected()) {
                is_reading_client = true; // soft-lock `getCommand()` return

                // clear buffer before writing
                String buffer = "";

                // wait client to start send data
                while (!client.available()) {}

                // Memory overflow resistance
                // (512 bytes is max per request)
                if (client.available() > 512) {
                    client.stop();
                }

                // loop until client send bytes
                while (client.available() > 0) {
                    char c = client.read();
                    buffer += c;
                }

                String separator = "\r\n\r\n"; // headers | body separator
                uint8_t pos = buffer.indexOf(separator);
                String body = buffer.substring(pos + separator.length());

                std::vector<uint8_t> enc {};
                std::vector<uint8_t> dec {};
                
                if (device_crypto.base64ToBytes(body, enc)) {
                    if (device_crypto.decrypt(enc, _pkey, dec)) {
                        Serial.print("🔵 ");
                        Serial.write(dec.data(), dec.size());
                        Serial.println();

                        // convert `vector` to `String`
                        std::vector<char> dec_chars(dec.begin(), dec.end());
                        dec_chars.push_back('\0');
                        String cmd_str(dec_chars.data());
                        
                        // store body treated as command into a specified command buffer
                        command_buffer = cmd_str;
                    }
                    else {
                        Serial.println("🟣 [BAD_DECRYPT]");
                    }
                }
                else {
                    Serial.println("🔴 BASE64_DECODE_ERR");
                }

                is_reading_client = false; // unlock `getCommand()` return
            }

            // at this moment client is still listening for a server response,
            // so to close the connection call `sendResponse(message)` method in the firmware code.
        }

        // check disconnects
        if (!WiFi.isConnected()) {
            Serial.println("🔴 Connection LOST");
            WiFi.disconnect(true); // close old connection
            delay(100);

            wifiBegin(); // try open a new one
            showStateMessage();
        }

        delay(1);
    }
    /**
    Sends a response message and closes the client's request.
    */
    void sendResponse(const char* message) {
        if (client) {
            // encrypt response message using private key
            std::vector<uint8_t> enc = device_crypto.encrypt(message, _pkey);

            String base64 = "";

            if (device_crypto.bytesToBase64(enc, base64)) {
                // send response
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: text/plain");
                client.println("Connection: close");
                client.println();
                client.println(base64);

                // close connection
                client.stop();

                // clear a command buffer
                command_buffer = "";
            }
            else {
                Serial.println("🔴 BASE64_ENCODE_ERR");
            }
            
        }
    }
    /**
    Sends a response message and closes the client's request.
    */
    void sendResponse(String message) {
        sendResponse(message.c_str());
    }
    /** 
    * Returns latest buffered command received from a client.
    */
    String getCommand() {
        if (is_reading_client) {
            return "";
        }
        else {
            return command_buffer;
        }
    }
    /**
    * Returns current device name. 
    */
    String getName() {
        return WiFi.getHostname();
    }
    /**
     * Returns device MAC address as `String`.
     */
    String getMAC() {
        return WiFi.macAddress();
    }
    /**
     * Returns current IP address as `String` (0.0.0.0 if device is offline).
     */
    String getIP() {
        IPAddress ip = WiFi.localIP();
        return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
    }

private:
    // wlan server instance
    WiFiServer server = WiFiServer(80);

    // wlan client container
    WiFiClient client {};

    // latest command received from client
    String command_buffer = "";

    // flag to determine whenever ddv server is reading client
    bool is_reading_client = false;

    // wifi hostname
    const char* _hostname = "";

    // wifi credentials ssid
    const char* _ssid = "";

    // wifi credentials password
    const char* _password  = "";

    // AES-256-GCM 32-bytes private key used in enc/dec process
    const char* _pkey = "";

    // Device setup object to init credentials data
    DeviceSetup device_setup {};
    // Device storage object to init credentials data
    DeviceStorage device_storage {};
    // Device storage object to init credentials data
    DeviceCrypto device_crypto {};

    // vector instances to keep data from nvs
    //
    std::vector<char> hostname_out {};
    std::vector<char> ssid_out {};
    std::vector<char> password_out {};
    std::vector<char> pkey_out {};

    /**
    * Attempts connect via WiFi interface using ssid and key specified with `set_wifi_credentials(wlan_ssid, wlan_key);`.
    */
    void wifiBegin(bool silent = false) {
        if (strlen(_ssid) == 0) {
            Serial.println("🔴 WiFi SSID cannot be empty");
            Serial.println("⚪ Send '/config' to serial to enter config menu");
            eternalSleep();
        }

        WiFi.mode(WIFI_STA);
        WiFi.begin(_ssid, _password);

        if (!silent) {
            Serial.print("🟡 Connecting to "); Serial.print(_ssid); Serial.println("...");
        }

        while (WiFi.status() != WL_CONNECTED) {
            // allow to enter device setup
            // during wifi setup
            device_setup.listen();

            if (WiFi.status() == WL_NO_SSID_AVAIL) {
                Serial.println("🔴 SSID is not available");
        
                // retry again
                wifiBegin();
                return;
            }

            if (WiFi.status() == WL_CONNECT_FAILED) {
                // retry again
                wifiBegin(true);
                return;
            }
            
            delay(100);
        }

        Serial.println("🟢 Connection OK");

        // start wlan server
        server.begin();
    }
    /**
    * Shows current connection state message.
    */
    void showStateMessage() {
        if (WiFi.isConnected()) { // show service info on success
            Serial.println("⚪ Device name: " + getName());
            Serial.println("⚪ IP: " + getIP());
            Serial.println("⚪ MAC: " + getMAC());
        }
        else { // print err message otherwise
            Serial.println("🔴 Connection cannot be established. Please check service messages above.");
        }
    }

    /**
    * Updates credentials by getting info from NVS.
    * `force` - Force fetch no matter of conditions.
    */
    void fetchWiFiData(const char*& hostname, const char*& ssid, const char*& password, bool force = false) {
        Serial.println("🟡 Fetching WiFi data...");
        
        if ((strlen(hostname) == 0 || force) && !device_storage.nvsRead("name", hostname_out)) {
            hostname = hostname_out.data();
        }
        if ((strlen(ssid) == 0 || force) && !device_storage.nvsRead("ssid", ssid_out)) {
            ssid = ssid_out.data();
        }
        if ((strlen(password) == 0 || force) && !device_storage.nvsRead("pass", password_out)) {
            password = password_out.data();
        }
    }

    /**
     * Generates private key and writes it into nvs.
     * Shows the key it the end *once*.
     * [It's impossible to see the key again after reboot]
     */
    void mkPrivateKey(String message) {
        Serial.println(message);

        String pkey = "";
        device_crypto.randHex32(pkey);

        device_storage.nvsWrite("pkey", pkey.c_str());

        Serial.println("㊙️ Private key (once): " + pkey);
    }

    /**
    * Updates private key data by getting info from NVS.
    * `pkey` - private key reference.
    */
    void fetchPrivateKey(const char*& pkey) {
        Serial.println("🟡 Fetching private key...");

        if (strlen(pkey) == 0) {
            esp_err_t pkey_err {};

            pkey_err = device_storage.nvsRead("pkey", pkey_out);
        
            if (!pkey_err) {
                pkey = pkey_out.data();
                // re-gen key since nvs data is invalid
                if (!device_crypto.isPrivateKeyValid(String(pkey))) {
                    mkPrivateKey("🟠 Private key is invalid, re-generating...");
                    fetchPrivateKey(pkey); // re-get value from memory
                    return;
                }
            }
            else if (pkey_err == ESP_ERR_NVS_NOT_FOUND) {
                mkPrivateKey("🟡 Generating private key...");
                fetchPrivateKey(pkey); // re-get value from memory
                return;
            }
        }
    }

    /**
     * Locks further app execution with a cool message.
     */
    void eternalSleep() {
        Serial.println("⚫ Entering eternal sleep mode...");
        while (true) {
            // allow to enter device setup
            // during eternal sleep
            device_setup.listen();
            delay(100);
        };
    }
};