#include <nvs.h>
#include <nvs_flash.h>

#ifndef DeviceStorage_H
#define DeviceStorage_H

class DeviceStorage {
public:
    /** Initializes NVS (Non-Volatile Storage). */
    void init() {
        esp_result = nvs_flash_init();
        if (esp_result) { // something went wrong
            esp_result = nvs_flash_erase();
            if (!esp_result) { // try to free storage and re-init
                esp_result = nvs_flash_init();
                if (!esp_result) {
                    Serial.println("🟢 NVS initialized");
                }
                else {
                    Serial.println("🔴 NVS initialization failed");
                }
            }
        }
        else {
            Serial.println("🟢 NVS initialized");
        }
    }
    /**
     * Writes data into NVS. Returns `ESP_OK` on success. 
     */
    esp_err_t nvsWrite(const char* key, const char* value) {
        nvs_handle_t nvs_handle;

        esp_result = nvs_open("storage", NVS_READWRITE, &nvs_handle);
        if (esp_result) return esp_result;

        esp_result = nvs_set_str(nvs_handle, key, value);
        if (esp_result) return esp_result;

        esp_result = nvs_commit(nvs_handle);
        if (esp_result) return esp_result;

        nvs_close(nvs_handle);

        return esp_result;
    }

    /**
     * Reads data from NVS. Returns `ESP_OK` on success. 
     */
    esp_err_t nvsRead(const char* key, std::vector<char>& out) {
        nvs_handle_t nvs_handle;

        esp_result = nvs_open("storage", NVS_READWRITE, &nvs_handle);
        if (esp_result) return esp_result;

        size_t len = 0;

        esp_result = nvs_get_str(nvs_handle, key, nullptr, &len);
        if (esp_result) return esp_result;

        std::vector<char> res(len);
        esp_result = nvs_get_str(nvs_handle, key, res.data(), &len);
        if (esp_result) return esp_result;

        out = res;

        nvs_close(nvs_handle);

        return esp_result;
    }

    /**
     * Removes key data from NVS. Returns `ESP_OK` on success. 
     */
    esp_err_t nvsRemove(const char* key) {
        nvs_handle_t nvs_handle;

        esp_result = nvs_open("storage", NVS_READWRITE, &nvs_handle);
        if (esp_result) return esp_result;

        esp_result = nvs_erase_key(nvs_handle, key);
        if (esp_result) return esp_result;

        esp_result = nvs_commit(nvs_handle);
        if (esp_result) return esp_result;

        nvs_close(nvs_handle);

        return esp_result;
    }

    /**
     * Removes all data from NVS. Returns `ESP_OK` on success. 
     */
    esp_err_t nvsClear() {
        nvs_handle_t nvs_handle;

        esp_result = nvs_open("storage", NVS_READWRITE, &nvs_handle);
        if (esp_result) return esp_result;

        esp_result = nvs_erase_all(nvs_handle);
        if (esp_result) return esp_result;

        esp_result = nvs_commit(nvs_handle);
        if (esp_result) return esp_result;

        nvs_close(nvs_handle);

        return esp_result;
    }

private:
    esp_err_t esp_result {};
};

#endif