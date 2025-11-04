#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Ethernet.h>
    
enum class State {
    Default,
    Ethn,   // Ethernet
    Wlan    // WLAN
};

/**
* Core network lib class to handle server side of de:things devices.
*/
class Delib {
public:
    /**
    * WLAN server used to handle wlan requests whenever WLAN is in use.
    */
    ESP8266WebServer wlan_server = ESP8266WebServer(80);
    /**
    * Ethernet server used to handle ethernet requests whenever Ethernet is in use.
    */
    EthernetServer ethn_server = EthernetServer(80);

    /**
    * Data buffer received from the client upon ethernet request to the ethernet server. 
    * `ethn_buffer` updates each tick so it need to be handled in `loop()` before `delib.update()` invoked.
    */
    String ethn_buffer = "";

    /**
    * Initialization method. Use it to start a proper server after pre-requirements have finished.
    */
    void init(uint8_t mac[6]) {
        Serial.begin(74880);
        while (!Serial) {}

        Serial.println();
        delay(2000);

        if (mac == bad_mac) {
            Serial.println("Cannot use MAC address specified. Initialization failed.");
            return;
        }

        Serial.println("Selected Ethernet profile.");

        delay(500);

        Ethernet.setMACAddress(mac);

        if (Ethernet.begin(mac) == 0) { // if establishing ethernet returned `0` (error), try to use wlan instead
            Serial.println("Cannot access network through ethernet. Is cable plugged in?");
            Serial.println();
            Serial.println("Selected WLAN profile."); 

            delay(500);

            if (wifi_ssid == "") { // return if no ssid specified.
                Serial.println("SSID is empty. Have you tried to use set_wifi_credentials(ssid, key) before init(mac) to define wifi credentials?"); 
                return;
            }

            WiFi.mode(WIFI_STA);
            wifi_set_macaddr(STATION_IF, mac);
            WiFi.begin(wifi_ssid, wifi_key); // begin the connection to specified wlan

            Serial.print("Connecting to "); Serial.println(wifi_ssid);

            while (WiFi.status() != WL_CONNECTED)
            {
                delay(500);
                Serial.print(".");
            }
            Serial.println();

            Serial.print("Connected, IP address: "); Serial.println(WiFi.localIP());

            // start wlan server
            wlan_server.begin();

            state = State::Wlan; // state to determine that wlan server is up
        }
        else { // ethernet connection established, so start `ethn_server`
            Serial.print("Connected, IP address: "); Serial.println(Ethernet.localIP());
            // start ethernet server
            ethn_server.begin();

            state = State::Ethn; // state to determine that ethernet server is up
        }
    }
    /**
    * Main loop of `delib`. Handles client<>server communication if established.
    */
    void update() {
        // ethernet client handling
        if (state == State::Ethn) {
            EthernetClient client = ethn_server.available();
            if (client) {
                while (client.connected()) {
                    // read client request data if available
                    if (client.available()) {
                        // read client data per byte
                        char c = client.read();

                        // if line sent by client has been ended, send a response
                        if (c == '\n') {
                            client.println("HTTP/1.1 200 OK");
                            client.println("Content-Type: text/plain");
                            client.println("Connection: close");
                            client.println();
                            client.println("OK");
                            
                            // print received data in serial [debug purposes]
                            Serial.println(ethn_buffer);

                            // clear client buffer
                            ethn_buffer = "";
                        }
                        else { // otherwise add data to a buffer
                            ethn_buffer += c;
                        }
                    }
                }
                delay(1);
                client.stop();
            }
        }
        // wlan client handling
        if (state == State::Wlan) {
            wlan_server.handleClient();
        }
        // if nothing to handle, print this message
        if (state == State::Default) { 
            Serial.println("No profile selected. Please check your firmware or service messages above.");
            delay(100000);
        }
    }
    /**
    * Returns true if client's wlan request is valid (POST and has a proper SECRET). 
    * Use it in wlan client handlers to decline unauthorized requests.
    * You can define device secret with a `set_secret(your_secret);`
    */
    bool auth_wlan_request() {
        // return if wlan server is offline
        if (state != State::Wlan) {
            return false;
        }
        if (wlan_server.method() == HTTP_POST) {
            Serial.println("Received POST");
        }
        else { // return user if received request has a wrong type 
            Serial.println("Wrong method");
            wlan_server.send(405, "text/plain", "Wrong method");
            return false;
        }

        if (wlan_server.arg("plain") == secret) {
            wlan_server.send(200, "text/plain", "Fine secret");
        }
        else { // return user if received secret is invalid. [check set_secret()]
            Serial.println("Bad secret");
            wlan_server.send(403, "text/plain", "Bad secret");
            return false;
        }
        return true;
    }
    /**
    * Sets a device wlan credentials. If not set, delib will use ethernet instead.
    */
    void set_wifi_credentials(String ssid, String key) {
        wifi_ssid = ssid;
        wifi_key  = key;
    }
    /**
    * Sets a device secret used to authorize wlan requests. (WHALE by default)
    */
    void set_secret(String new_secret) {
        secret = new_secret;
    }
private:
    // secret to validate wlan requests
    String secret = "WHALE";

    State state = State::Default;

    uint8_t bad_ip[4];
    uint8_t bad_mac[6];

    // ssid, key
    String wifi_ssid = "";
    String wifi_key  = "";
};
