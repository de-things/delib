#include <WiFi.h>
#include <LiquidCrystal_I2C.h>

/**
* Core network lib class to handle server side of de:things devices based on ESP32 family.
*/
class Delib32 {
public:
    /**
    * WLAN server instance. Use it to handle events.
    */
    WiFiServer server = WiFiServer(80);

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
        
        Serial.begin(115200);
        while (!Serial) { lcd.print("."); }

        Serial.println("");

        lcd_print("[SERIAL]", "OK", 1000);
        Serial.println("Serial initialized");

        lcd_print("[SELECTED]", "WLAN", 2000);
        Serial.println("Selected WLAN profile."); 

        WiFi.mode(WIFI_STA);
        WiFi.hostname(device_name);
        WiFi.macAddress(mac);

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
            client = server.available();

            if (client) {
                if (client.connected()) {
                    // clear buffer before writing
                    String buffer = "";

                    // wait client to start send data
                    while (!client.available()) {}

                    // loop until client avaliable
                    while (client.available() > 0) {
                        char c = client.read();
                        buffer += c; // store chars into a buffer

                        // at this moment buffer fills up with all of request content:
                        // http 1.1
                        // headers: ...
                        // ...
                        // body: ...
                        
                        // so!
                        if (c == cmd_char) { // if found a command char (body section)...
                            buffer = ""; // ...clear buffer to fill it with appropriate command
                        }
                    }
                    
                    command = "!" + buffer; // save command into a specified variable
                    Serial.println("Received: !" + String(buffer)); // print buffered command in serial
                }
            }

            if (WiFi.status() == WL_DISCONNECTED || WiFi.status() == WL_CONNECTION_LOST) {
                // try to reconnect using wlan interface
                wifi_begin();
                show_state_message();
            }
        }
        else {
            // try to reconnect using wlan interface
            wifi_begin();
            show_state_message();
        }
        delay(1);
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
    Sends a response message and closes the client's request.
    */
    void send_response(String res_msg) {
        if (client) {
            // send response
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.println();
            client.println(res_msg);

            // disconnect client
            client.stop();
        }
    }
    /**
    * Converts ip[] to String.
    */
    String ip_to_string(IPAddress ip) {
        return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
    }
    /** 
    * Returns latest buffered command received from a client.
    */
    String get_command() {
        if (is_reading_client) {
            return "";
        }
        else {
            return command;
        }
    }
    /**
     * Sets a response message to send back to a client upon `http` request.
     */
    void set_response(String message) {
        response = message;
    }
private:
    // 州
    enum class State { Default, Wlan };

    // wlan client to handle incoming requests
    WiFiClient client {};

    // state handler
    State state = State::Default;

    String device_name = "Cardboard";

    // --- lcd screen ---
    LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0,0,0);
    
    byte lcd_addr = 0x3F;
    int  lcd_cols = 16;
    int  lcd_rows = 2;
    // --- 

    // latest command received from client
    String command = "";

    // response message to send to a client
    String response = "";

    // char used to indentify command's name begin
    char cmd_char = '!';

    // flag to determine whenever ddv server is reading client
    bool is_reading_client = false;

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
            if (WiFi.status() == WL_NO_SSID_AVAIL) {
                lcd_print("[ERR] FAILED", "BAD SSID", 5000);
                Serial.println("\nSSID is not available.");

                // retry again
                wifi_begin();
                return;
            }

            if (WiFi.status() == WL_CONNECT_FAILED) {
                lcd_print("[ERR]", "UNKNOWN", 5000);
                Serial.println("\nSomethinig went wrong. Is credentials valid?");

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
