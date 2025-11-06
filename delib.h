#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Ethernet.h>
#include <LiquidCrystal_I2C.h>
    
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
    * `mac` - device mac.
    */
    void init(byte mac[6], String device_name) {
        lcd.init();
        lcd.backlight();

        lcd_print("[DE:THINGS]", device_name, 2000);

        lcd_print("[SERIAL]", "", 10);
        
        Serial.begin(74880);
        while (!Serial) {
            lcd.print(">");
        }

        lcd_print("[SERIAL]", "OK", 1000);
        Serial.println("Serial initialized");

        lcd_print("[PROFILE]", "Ethernet", 2000);
        Serial.println("Selected Ethernet profile.");

        if (Ethernet.hardwareStatus() == EthernetNoHardware) { // if no ethernet hardware, try to use wlan instead
            lcd_print("[ERR]", "No eth hardware", 2000);
            Serial.println("Ethernet hardware is not found.");

            // call wlan init
            wlan_init(mac);
        }
        else if (Ethernet.begin(mac) == 0) { // if establishing ethernet returned `0` (error), try to use wlan instead
            lcd_print("[ERR]", "No cable found", 2000);
            Serial.println("Cannot access network through ethernet. Is cable plugged in?");
            Serial.println();
            
            // call wlan init
            wlan_init(mac);
        }
        else { // ethernet connection established, so start `ethn_server`
            lcd_print("[ETH]", "OK", 1000);

            lcd_print("[IP]", ip_to_string(Ethernet.localIP()), 10);
            Serial.print("Connected, IP address: "); Serial.println(Ethernet.localIP());

            // start ethernet server
            ethn_server.begin();

            state = State::Ethn; // state to determine that ethernet server is up
        }
    }
    /**
    * Initialization method for WLAN Profile. Use `delib.init()` instead if you want to check and init ethernet profile as well. 
    * If `init()` fails to initialize ethernet profile, this method will be called automatically.
    */
    void wlan_init(byte mac[]) {
        lcd_print("[PROFILE]", "WLAN", 2000);
        Serial.println("Selected WLAN profile."); 

        delay(500);

        if (wifi_ssid == "") { // return if no ssid specified.
            lcd_print("[ERR]", "Empty SSID", 2000);
            Serial.println("SSID is empty. Have you tried to use set_wifi_credentials(ssid, key) before init(mac) to define wifi credentials?"); 
            return;
        }

        WiFi.mode(WIFI_STA);
        wifi_set_macaddr(STATION_IF, mac);
        WiFi.begin(wifi_ssid, wifi_key); // begin the connection to specified wlan

        lcd_print(wifi_ssid, "", 10);
        Serial.print("Connecting to "); Serial.println(wifi_ssid);

        while (WiFi.status() != WL_CONNECTED)
        {
            lcd.print(">");
            Serial.print(".");
            delay(500);
        }
        Serial.println();

        lcd_print("[WLAN]", "OK", 1000);

        lcd_print("[IP]", ip_to_string(WiFi.localIP()), 10);
        Serial.print("Connected, IP address: "); Serial.println(WiFi.localIP());

        // start wlan server
        wlan_server.begin();

        state = State::Wlan; // state to determine that wlan server is up
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
            lcd_print("[ERR]", "No profile", 10);
            Serial.println("No profile selected. Please check your firmware or service messages above.");
            delay(100000);
        }
        delay(1);
    }
    /**
    * Returns true if client's wlan request is valid (POST and has a proper SECRET). 
    * Use it in wlan client handlers to decline unauthorized requests.
    * You can define device secret with a `set_secret(your_secret);`
    * `response` - message to be sent if auth confirmed ("OK" by default).
    */
    bool auth_wlan_request(String response = "") {
        // return if wlan server is offline
        if (state != State::Wlan) {
            return false;
        }
        if (wlan_server.method() == HTTP_POST) {
            Serial.println("RECEIVED POST");
        }
        else { // return user if received request has a wrong type 
            Serial.println("WRONG METHOD");
            wlan_server.send(405, "text/plain", "WRONG METHOD");
            return false;
        }

        if (wlan_server.arg("plain") == secret) {
            if (response == "") {
                wlan_server.send(200, "text/plain", "OK");
            }
            else {
                wlan_server.send(200, "text/plain", response);
            }
        }
        else { // return user if received secret is invalid. [check set_secret()]
            Serial.println("BAD SECRET");
            wlan_server.send(403, "text/plain", "ERROR");
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
    /**
    * Prints something on lcd screen. 
    * `row_0` - message to first row;
    * `row_1` - message to second row;
    * `delay_time` - time to wait before continue firmware execution.
    */
    void lcd_print(String row_0, String row_1, int delay_time) {
        lcd_clear();

        lcd.setCursor(0, 0);
        lcd.print(row_0);
        lcd.setCursor(0, 1);
        lcd.print(row_1);

        delay(delay_time);
    }
    /**
    * Clears lcd screen. 
    */
    void lcd_clear() {
        lcd.setCursor(0, 0);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print("                ");
    }
    /**
    * Converts ip[] to String.
    */
    String ip_to_string(IPAddress ip)
    {
    return String(ip[0]) + "." + 
            String(ip[1]) + "." + 
            String(ip[2]) + "." + 
            String(ip[3]);
    }
private:
    LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x3F, 16, 2);

    // secret to validate wlan requests
    String secret = "WHALE";

    State state = State::Default;

    // ssid, key
    String wifi_ssid = "";
    String wifi_key  = "";
};
