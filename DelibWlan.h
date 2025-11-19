#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LiquidCrystal_I2C.h>

/**
* Core network lib class to handle server side of de:things devices.
*/
class DelibWlan {
public:
    /**
    * WLAN server instance. Use it to handle events.
    */
    ESP8266WebServer server = ESP8266WebServer(80);

    /**
    * Initialization method. Use it to start a proper server after pre-requirements have finished.
    * `mac` - Any MAC address for a device.
    */
    void init(byte mac[6]) {
        lcd = LiquidCrystal_I2C(lcd_addr, lcd_cols, lcd_rows);

        lcd.init();
        lcd.backlight();

        lcd_print("[INIT]", device_name, 2000);

        lcd_print("[SERIAL]", "", 10);
        
        Serial.begin(74880);
        while (!Serial) { lcd.print("."); }

        lcd_print("[SERIAL]", "OK", 1000);
        Serial.println("Serial initialized");

        lcd_print("[SELECTED]", "WLAN", 2000);
        Serial.println("Selected WLAN profile."); 

        WiFi.mode(WIFI_STA);
        WiFi.hostname(device_name);

        wifi_set_macaddr(STATION_IF, mac);

        // establish connection to a specified wlan
        wifi_begin();

        // show state message after wlan initialization
        show_state_message();
    }
    /**
    * Main loop of `delib`. Handles client<>server communication if established.
    */
    void update() {
        // wlan client handling
        if (state == State::Wlan) {
            server.handleClient();

            // handle wifi down and reconnect if so:
            if (WiFi.status() != WL_CONNECTED) {
                state = State::Default;

                lcd_print("[ERR]", "DISCONNECTED", 2000);
                Serial.println("Disconnected.");

                // establish connection to a specified wlan
                wifi_begin();

                // show state message after wlan re-initialization
                show_state_message();
            }
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
        if (server.method() == HTTP_POST) {
            Serial.println("RECEIVED POST");
        }
        else { // return user if received request has a wrong type 
            Serial.println("WRONG METHOD");
            server.send(405, "text/plain", "WRONG METHOD");
            return false;
        }

        if (server.arg("plain") == secret) {
            if (response == "") {
                server.send(200, "text/plain", "OK");
            }
            else {
                server.send(200, "text/plain", response);
            }
        }
        else { // return user if received secret is invalid. [check set_secret()]
            Serial.println("BAD SECRET");
            server.send(403, "text/plain", "ERROR");
            return false;
        }
        return true;
    }
    /**
    * Sets a device wlan credentials. If not set, delib will use ethernet instead.
    * `ssid` - WiFi name; 
    * `key` - WiFi password.
    */
    void set_wifi_credentials(String wlan_ssid, String wlan_key) {
        ssid = wlan_ssid;
        key  = wlan_key;
    }
    /**
    * Sets a device secret used to authorize wlan requests. 
    * `new_secret` - Any secret phrase to be used by `auth_wlan_request(response);` (WHALE by default).
    */
    void set_secret(String new_secret) {
        secret = new_secret;
    }
    /**
    * Sets device name. 
    * `name` - Any name for a device (Cardboard by default).
    */
    void set_device_name(String name) {
        device_name = name;
    }
    /**
    * Sets attributes for lcd screen connected to the controller.
    * `addr` - logical address to send and show data on screen (1602 16x2 lcd screen owns 0x3F address for this);
    * `cols` - number of columns screen owns;
    * `rows` - number of rows screen owns.
    */
    void set_lcd_attributes(byte addr, int cols, int rows) {
        lcd_addr = addr;
        lcd_cols = cols;
        lcd_rows = rows;
    }
    /**
    * Prints something on lcd screen. 
    * `row_0` - message to first row;
    * `row_1` - message to second row;
    * `delay_time` - time to wait before continue firmware execution.
    */
    void lcd_print(String row_0, String row_1, int delay_time) {
        lcd.clear();

        lcd.setCursor(0, 0);
        lcd.print(row_0);
        lcd.setCursor(0, 1);
        lcd.print(row_1);

        delay(delay_time);
    }
    /**
    * Converts ip[] to String.
    */
    String ip_to_string(IPAddress ip) {
        return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
    }
private:
    /**
    * 金属片の州
    */
    enum class State { Default, Wlan };

    // state handler
    State state = State::Default;

    String device_name = "Cardboard";

    // --- lcd screen ---
    LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0,0,0);
    
    byte lcd_addr = 0x3F;
    int lcd_cols = 16;
    int lcd_rows = 2;
    // --- 

    // secret to validate wlan requests
    String secret = "WHALE";

    // ssid, key
    String ssid = "";
    String key  = "";

    // animation frame for lcd connection process
    int anim_frame = 0;

    /**
    * Attempts connect via WiFi interface using ssid and key specified with `set_wifi_credentials(wlan_ssid, wlan_key);`.
    */
    void wifi_begin() {
        WiFi.begin(ssid, key);

        lcd_print(ssid, "", 10);
        Serial.print("Connecting to "); Serial.println(ssid);

        while (WiFi.status() != WL_CONNECTED)
        {
            if (WiFi.status() == WL_WRONG_PASSWORD) {
                lcd_print("[ERR]", "INVALID KEY", 5000);
                Serial.println("\nKey is invalid. Connection failed.");
                return;
            }

            if (WiFi.status() == WL_NO_SSID_AVAIL) {
                lcd_print("[ERR] IS 2.4GHZ?", "NO SSID AVAIL", 5000);
                Serial.println("\nSSID is not available. Is it 2.4GHz? Connection failed.");

                // retry again
                wifi_begin();
                return;
            }

            if (WiFi.status() == WL_CONNECT_FAILED) {
                lcd_print("[ERR]", "FAILED", 5000);
                Serial.println("\nSomethinig went wrong. Connection failed.");

                // retry again
                wifi_begin();
                return;
            }

            // dots animation for connection process
            if (anim_frame >= 3) {
                lcd.setCursor(0, 1);
                lcd.print("   ");
                lcd.setCursor(0, 1);
                anim_frame = 0;
            }
            else {
                lcd.print(".");
                anim_frame += 1;
            }

            Serial.print(".");
            delay(200);
        }

        lcd_print("[WLAN]", "OK", 1000);

        state = State::Wlan; // state to determine that wlan server is up

        // start wlan server
        server.begin();
    }
    /**
    * Shows current connection state message.
    */
    void show_state_message() {
        if (state == State::Wlan) {
            lcd_print("[IP] " + device_name, ip_to_string(WiFi.localIP()), 1);
            Serial.print("\nConnected, IP address: "); Serial.println(WiFi.localIP());
        }
        else if (state == State::Default) { // print this message otherwise
            lcd_print("[ERR]", "NO CONNECTION", 10);
            Serial.println("Connection cannot be established. Please check service messages above.");
        }
    }
};
