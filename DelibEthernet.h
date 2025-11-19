#include <Ethernet.h>
#include <LiquidCrystal_I2C.h>

/**
* Core network lib class to handle server side of de:things devices.
*/
class DelibEthernet {
public:
    /**
    * Initialization method. Use it to start a proper server after pre-requirements have finished.
    * `mac` - Any MAC address for a device.
    */
    void init(byte mac[6]) {
        _mac[0] = mac; // save mac data for later

        lcd = LiquidCrystal_I2C(lcd_addr, lcd_cols, lcd_rows);

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
            String buffer = "";
            EthernetClient client = server.available();

            if (client) {
                while (client.connected()) {
                    if (client.available() > 0) {
                        // set flag to true to let lib know, 
                        // that server is processing a request
                        is_reading_client = true;

                        char c = client.read();
                        buffer += c;

                        if (c == cmd_char) {
                            Serial.print("Received: !");
                            buffer = ""; // clear buffer to fill it with a command content only (was headers stuff before like: content-type, http 1.1, etc. etc.)
                        }
                    }
                    else {
                        Serial.println(buffer); // show received command in logs

                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-Type: text/plain");
                        client.println("Connection: close");
                        client.println();
                        client.println("OK");

                        command = buffer; // store command in specified variable

                        is_reading_client = false; // remove reading flag

                        break;
                    }
                }

                delay(1);
                client.stop();
            }

            // detect disconnect 
            // only for W5200, W5500 Ethernet interfaces. 
            // `Ampreka` ethernet shield use something unique, so it doesn't work with it,
            // but `iarduino` shield for an example has W5500 configuration so I suppose it works this way.
            if (Ethernet.linkStatus() == LinkOFF) {
                lcd_print("[ERR]", "DISCONNECTED", 2000);
                Serial.println("Connection lost.");
                state == State::Default;
            }
        }
        else {
            // try to reconnect using ethernet interface
            eth_begin(_mac);
        }
        delay(1);
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
    * Sets device name. 
    * `name` - Any name for a device (Cardboard by default).
    */
    void set_device_name(String name) {
        device_name = name;
    }
    /**
    * Sets attributes for lcd screen connected to the controller.
    * `addr` - logical address to send and show data on screen (1602 16x2 lcd screen uses 0x3F address for this);
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
    enum class State { Default, Ethernet };

    // state handler
    State state = State::Default;

    String device_name = "Cardboard";

    // --- lcd screen ---
    LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0,0,0);
    
    byte lcd_addr = 0x3F;
    int lcd_cols = 16;
    int lcd_rows = 2;
    // --- 

    byte _mac[6];
    EthernetServer server = EthernetServer(80);

    String command = "";

    // char used to indentify command's name begin
    char cmd_char = '!';

    bool is_reading_client = false;
    

    /**
    * Attempts connect via Ethernet interface.
    */
    void eth_begin(byte mac[6]) {
        lcd_print("[STATUS]", "CONNECTING...", 1);
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

    /**
    * Shows current connection state message.
    */
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