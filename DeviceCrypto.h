#include <esp_random.h>
#include <string.h>

#ifndef DeviceCrypto_H
#define DeviceCrypto_H

class DeviceCrypto {
public:
    /**
     * Generates random string of 32 chars.
     */
    void randHex32(String& out) {
        uint8_t bytes[16] {};
        out = "";

        esp_fill_random(bytes, sizeof(bytes));

        const char* cset = "0123456789abcdef";

        for (uint8_t b : bytes) {
            out += cset[b / 16];
            out += cset[b % 16];
        }
    }

    /**
     * Returns true if private key specified is valid for use in encryption.
     */
    bool isPrivateKeyValid(String pkey) {
        // non-equal to 32 bytes
        if (strlen(pkey.c_str()) != 32) return false;

        pkey.toLowerCase();
        
        const char cset[] = "0123456789abcdef";
        for (int i = 0; i < pkey.length(); ++i) {
            if (strchr(cset, pkey[i]) == nullptr)
                return false;
        }
        return true;
    }

    /**
    * Encrypts data string with a private key by AES-256-GCM.
    * `data` - Data string to encrypt;
    * `pkey` - Private key string.
    * Returns encrypted data `vector<uint8_t>` on success, empty `vector<uint8_t>` otherwise.
    */
    std::vector<uint8_t> encrypt(const char* data, const char* pkey) {
        const unsigned char* _pkey = reinterpret_cast<const unsigned char*>(pkey);
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

        int rc = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, _pkey, 256);

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
    * Decrypts `enc` vector with a private key (pkey) by AES-256-GCM.
    * Returns decrypted data string on success, empty `vector` otherwise.
    * `enc` - an encrypted data to decrypt;
    * `pkey` - Private key string;
    * `output` - an output `vector<uint8_t>` reference to store decrypted data.
    */
    bool decrypt(const std::vector<uint8_t>& enc, const char* pkey, std::vector<uint8_t>& output) {
        const unsigned char* _pkey = reinterpret_cast<const unsigned char*>(pkey);

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

        int rc = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, _pkey, 256);

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
     * Converts base64 to `vector<uint8_t>`.
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

private:
    // IV length in bytes
    const uint8_t iv_len  = 12;
    
    // GSM tag length in bytes (for AES-GCM should be 128-bit) 
    const uint8_t tag_len = 16;
};

#endif