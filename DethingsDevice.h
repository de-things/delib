#include <WiFi.h>

/**
* Core network lib class to handle server side of de:things devices based on ESP32 family.
*/
class DethingsDevice {
public:
    /**
     * Dethings device instance.
     * `deviceName` - device preferable hostname;
     * `ssid` - WiFi name to connect to;
     * `key` - WiFi password to connect with.
     */
    DethingsDevice(const char* deviceName, String ssid, String key) {
        WiFi.setHostname(deviceName);
        WiFi.setAutoReconnect(true);

        _ssid = ssid;
        _key = key;
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
        client = server.available();
        
        if (client) {
            if (client.connected()) {
                isReadingClient = true; // soft-lock `getCommand()` return

                // clear buffer before writing
                String requestBuffer = "";

                // wait client to start send data
                while (!client.available()) {}

                // loop until client avaliable
                while (client.available() > 0) {
                    char c = client.read();
                    requestBuffer += c; // store chars into a buffer

                    // at this moment buffer fills up with all of request content:
                    // http 1.1
                    // headers: ...
                    // ...
                    // body: ...
                    
                    // so!
                    if (c == commandIdentifier) { // if found a command char (body section)...
                        requestBuffer = ""; // ...clear buffer to fill it with appropriate command
                    }
                }
                
                commandBuffer = "!" + requestBuffer; // save command into a specified variable
                Serial.println("Called: !" + String(requestBuffer)); // print buffered command in serial

                isReadingClient = false; // unlock `getCommand()` return
            }

            // client here is still connected to server,
            // so to close the connection call `send_response()` method in firmware code.
        }

        delay(1);
    }
    /**
    Sends a response message and closes the client's request.
    */
    void sendResponse(String message) {
        if (client) {
            // send response
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.println();
            client.println(message);

            // disconnect client
            client.stop();

            // clear the command buffer
            commandBuffer = "";
        }
    }
    /** 
    * Returns latest buffered command received from a client.
    */
    String getCommand() {
        if (isReadingClient) {
            return "";
        }
        else {
            return commandBuffer;
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
        return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);;
    }

private:
    // wlan server instance
    WiFiServer server = WiFiServer(80);

    // wlan client container
    WiFiClient client {};

    // latest command received from client
    String commandBuffer = "";

    // char used to indentify command's name begin
    char commandIdentifier = '!';

    // flag to determine whenever ddv server is reading client
    bool isReadingClient = false;

    // wifi credentials ssid
    String _ssid = "";

    // wifi credentials key
    String _key  = "";

    /**
    * Attempts connect via WiFi interface using ssid and key specified with `set_wifi_credentials(wlan_ssid, wlan_key);`.
    */
    void wifiBegin() {
        WiFi.begin(_ssid, _key);

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
        if (WiFi.isConnected()) { // show ip on success
            Serial.println("Device Name: " + getName());
            Serial.println("IP: " + getIP());
            Serial.println("MAC: " + getMAC());
        }
        else { // print this message otherwise
            Serial.println("Connection cannot be established. Please check service messages above.");
        }
    }
};