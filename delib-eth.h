#include <Ethernet.h>
#include <LiquidCrystal_I2C.h>
    
enum class State { Default, Ethernet };

/**
* Core network lib class to handle server side of de:things devices.
*/
class DelibEthernet {
public:
    /**
    * WLAN server used to handle wlan requests whenever WLAN is in use.
    */
    EthernetServer server = EthernetServer(80);

    /**
    * Initialization method. Use it to start a proper server after pre-requirements have finished.
    * `mac` - device mac.
    */
    void init(byte mac[6]) {
        lcd.init();
        lcd.backlight();

        lcd_print("[INIT]", device_name, 2000);

        lcd_print("[SERIAL]", "", 10);
        
        Serial.begin(74880);
        while (!Serial) { lcd.print("."); }

        lcd_print("[SERIAL]", "OK", 1000);
        Serial.println("Serial initialized");

        lcd_print("[SELECTED]", "ETH", 2000);
        Serial.println("Selected ETH profile."); 

        // establish connection via eth
        eth_begin(mac);

        // show state message after eth init
        show_state_message();
    }
    /**
    * Main loop of `delib`. Handles client<>server communication if established.
    */
    void update() {
        // wlan client handling
        if (state == State::Ethernet) {
            EthernetClient client = server.available();
            if (client) {
                buffer += client.read();
            }
            else {
                if (buffer != "") {
                    old_buffer = buffer;
                    // "clear" buffer after clonning a value to `old_buffer`
                    buffer = "";
                }
                client.stop();
            }
        }
        delay(1);
    }
    /** 
    * Points to the last received buffered message from a client. 
    * Returns `false` if client is sending something at the moment,
    * `true` otherwise.
    */
    bool get_buffer(String* result) {
        if (server.available()) {
            return false;
        }
        else { // set result as buffer if client is not sending anything
            result = &old_buffer;
            // "clear" `old_buffer` after pointing to the `result`
            old_buffer = "";
        }
        return true;
    }
    /**
    * Sets device name. 
    */
    void set_device_name(String name) {
        device_name = name;
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
    String ip_to_string(IPAddress ip) {
        return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
    }
private:
    LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x3F, 16, 2);

    State state = State::Default;

    String device_name = "Cardboard";

    String buffer = "";
    String old_buffer = "";

    /**
    * Attempts connect to Eth.
    */
    void eth_begin(byte mac[6]) {
        Serial.println("Connecting...");
        if (Ethernet.begin(mac) == 0)
        {
            lcd_print("[ERR]", "NO CABLE", 2000);
            Serial.println("Cannot connect. Check if Ethernet cable is plugged in.");
        }
        else {
            lcd_print("[ETH]", "OK", 1000);
            // state to determine that ethernet is ok
            state = State::Ethernet;
            // start eth server
            server.begin();
        }
    }

    void show_state_message() {
        if (state == State::Ethernet) {
            lcd_print("[IP] " + device_name, ip_to_string(Ethernet.localIP()), 1);
            Serial.print("\nConnected, IP address: "); Serial.println(Ethernet.localIP());
        }
        else if (state == State::Default) { // print this message otherwise
            lcd_print("[ERR]", "NO CONNECTION", 10);
            Serial.println("Connection cannot be established. Please check service messages above.");
        }
    }
};
