#include <WiFi.h>
#include <Arduino.h>
#include <vector>
#include <mbedtls/gcm.h>
#include <mbedtls/base64.h>

/**
* Core network lib class to handle server side of de:things devices based on ESP32 family.
*/
class DethingsDevice {
public:
    /**
     * Dethings device instance.
     * `device_name` - device preferable hostname;
     * `ssid` - WiFi name to connect to;
     * `password` - WiFi password to connect with;
     * `secret` - 32-byte private key (secret) used in AES-256-GCM encryption.
     */
    DethingsDevice(const char* device_name, String ssid, String password, const char* secret) {
        WiFi.setHostname(device_name);
        WiFi.setAutoReconnect(true);

        _ssid       = ssid;
        _password   = password;
        _secret     = secret;
    }

    /**
    * Initialization method. Use it to start a proper server after pre-requirements have finished.
    * `mac` - Any MAC address for a device.
    */
    void init() {
        Serial.begin(115200);
        while (!Serial) { }
        
        Serial.println("\nSerial initialized...");

        // establish connection to a specified wlan
        wifiBegin();
        // show state message after wlan initialization
        showStateMessage();
    }
    /**
    * Main loop of `delib`. Handles client<>server communication if established.
    */
    void update() {
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
                
                if (base64ToBytes(body, enc)) {
                    if (decrypt(enc, dec)) {
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
                        Serial.println("[Decrypt error]");
                    }
                }
                else {
                    Serial.println("[base64 decode error]");
                }

                is_reading_client = false; // unlock `getCommand()` return
            }

            // at this moment client is still listening for a server response,
            // so to close the connection call `sendResponse(message)` method in the firmware code.
        }

        delay(1);
    }
    /**
    Sends a response message and closes the client's request.
    */
    void sendResponse(const char* message) {
        if (client) {
            // encrypt response message by a stored secret
            std::vector<uint8_t> enc = encrypt(message);

            String base64 = "";

            if (bytesToBase64(enc, base64)) {
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
                Serial.println("[base64 encode error]");
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

    // wifi credentials ssid
    String _ssid = "";

    // wifi credentials password
    String _password  = "";

    // AES-256-GCM 32-bytes key used in enc/dec process
    const char* _secret = "";

    // IV length in bytes
    const uint8_t iv_len  = 12;
    
    // GSM tag length in bytes (for AES-GCM should be 128-bit) 
    const uint8_t tag_len = 16;

    /**
    * Attempts connect via WiFi interface using ssid and key specified with `set_wifi_credentials(wlan_ssid, wlan_key);`.
    */
    void wifiBegin() {
        WiFi.begin(_ssid, _password);

        Serial.println("Connecting to " + _ssid);

        while (WiFi.status() != WL_CONNECTED) {
            if (WiFi.status() == WL_NO_SSID_AVAIL) {
                Serial.println("\nSSID is not available.");
        
                // retry again
                wifiBegin();
                return;
            }

            if (WiFi.status() == WL_CONNECT_FAILED) {
                Serial.println("\nSomethinig went wrong. Is credentials valid?");

                // retry again
                wifiBegin();
                return;
            }

            Serial.print(".");
            delay(200);
        }

        Serial.println();

        // start wlan server
        server.begin();
    }
    /**
    * Shows current connection state message.
    */
    void showStateMessage() {
        if (WiFi.isConnected()) { // show service info on success
            Serial.println("Device name: " + getName());
            Serial.println("IP: " + getIP());
            Serial.println("MAC: " + getMAC());
        }
        else { // print err message otherwise
            Serial.println("Connection cannot be established. Please check service messages above.");
        }
    }

    /**
    * Encrypts `data` string using stored secret bytes by AES-256-GCM.
    * Returns encrypted data `vector<uint8_t>` on success, empty `vector<uint8_t>` otherwise.
    */
    std::vector<uint8_t> encrypt(const char* data) {
        const unsigned char* secret = reinterpret_cast<const unsigned char*>(_secret);
        const size_t data_len = strlen(data);

        std::vector<uint8_t> enc(iv_len + data_len + tag_len);

        uint8_t* iv     = enc.data();
        uint8_t* cipher = enc.data() + iv_len;
        uint8_t* tag    = cipher + data_len;

        // Generate a fresh IV for every encryption.
        for (size_t i = 0; i < iv_len; i += 4) {
            uint32_t rnd_word = esp_random();
            size_t count = min((size_t)4, iv_len - i);
            memcpy(iv + i, &rnd_word, count);
        }

        mbedtls_gcm_context ctx {};
        mbedtls_gcm_init(&ctx);

        int rc = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, secret, 256);

        if (rc == 0) {
            rc = mbedtls_gcm_crypt_and_tag(
                &ctx,
                MBEDTLS_GCM_ENCRYPT,
                data_len,
                iv, iv_len,
                nullptr, 0,
                reinterpret_cast<const uint8_t*>(data),
                cipher,
                tag_len, tag
            );
        }

        mbedtls_gcm_free(&ctx);

        if (rc != 0) {
            enc.clear();
        }

        return enc;
    }

    /**
    * Decrypts `enc` vector using stored secret bytes by AES-256-GCM.
    * Returns decrypted data string on success, empty `vector` otherwise.
    * `enc` - an encrypted data to decrypt;
    * `output` - an output `vector<uint8_t>` reference to store decrypted data.
    */
    bool decrypt(const std::vector<uint8_t>& enc, std::vector<uint8_t>& output) {
        const unsigned char* secret = reinterpret_cast<const unsigned char*>(_secret);

        if (enc.size() < iv_len + tag_len) {
            return false;
        }

        const uint8_t*  iv          = enc.data();
        const uint8_t*  cipher      = enc.data() + iv_len;
        const size_t    cipher_len  = enc.size() - iv_len - tag_len;
        const uint8_t*  tag         = enc.data() + iv_len + cipher_len;

        output.resize(cipher_len);

        mbedtls_gcm_context ctx {};
        mbedtls_gcm_init(&ctx);

        int rc = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, secret, 256);

        if (rc == 0) {
            rc = mbedtls_gcm_auth_decrypt(
                &ctx,
                cipher_len,
                iv, iv_len,
                nullptr, 0,
                tag, tag_len,
                cipher,
                output.data()
            );
        }

        mbedtls_gcm_free(&ctx);

        if (rc != 0) {
            output.clear();
            return false;
        }

        return true;
    }

    /**
     * Decodes string data to `vector<uint8_t>`.
     * `data` - an original base64 data to decode.
     * `output` - a `vector<uint8_t>` reference to store decoded data.
     */
    bool base64ToBytes(const String& data, std::vector<uint8_t>& output) {
        output.resize((data.length() / 4) * 3 + 3);

        size_t out_len = 0;
        int rc = mbedtls_base64_decode(
            output.data(),
            output.size(),
            &out_len,
            reinterpret_cast<const unsigned char*>(data.c_str()),
            data.length()
        );

        if (rc != 0) {
            output.clear();
            return false;
        }

        output.resize(out_len);
        return true;
    }
    bool bytesToBase64(const std::vector<uint8_t>& enc, String& output) {
        std::vector<unsigned char> buffer(((enc.size() + 2) / 3) * 4 + 1);

        size_t enc_len = 0;
        int rc = mbedtls_base64_encode(
            buffer.data(),
            buffer.size(),
            &enc_len,
            enc.data(),
            enc.size()
        );

        if (rc != 0) {
            return false;
        }

        buffer[enc_len] = '\0';
        output = reinterpret_cast<const char*>(buffer.data());
        return true;
    }
};