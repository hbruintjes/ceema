/**
 * Copyright 2017 Harold Bruintjes
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "backup.h"

#include <encoding/base32.h>
#include <encoding/sha256.h>
#include <openssl/evp.h>
#include <logging/logging.h>

namespace ceema {

    const unsigned HASH_LEN = 2u;
    const unsigned SALT_LEN = 8u;
    const unsigned PBKDF_ITER = 100000u;

    std::string make_backup(Account const& client, std::string const& password) {
        byte_array<SALT_LEN + CLIENTID_SIZE + crypto_box_SECRETKEYBYTES + HASH_LEN> data;
        randombytes_buf(data.data(), SALT_LEN);
        auto iter = data.begin() + SALT_LEN;
        iter = std::copy(client.id().begin(), client.id().end(), iter);
        iter = std::copy(client.sk().begin(), client.sk().end(), iter);

        byte_array<hash_256_len> hash = hash_sha256(data.begin()+SALT_LEN, CLIENTID_SIZE + crypto_box_SECRETKEYBYTES);
        iter = std::copy(hash.begin(), hash.begin()+2, iter);

        // pbkdf2+HMAC hash the password, salt with client ID. Result is encryption key
        byte_array<crypto_stream_KEYBYTES> enckey;
        const EVP_MD* evp_sha256 = EVP_sha256();
        int res = PKCS5_PBKDF2_HMAC(password.data(), password.size(), data.data(), SALT_LEN, PBKDF_ITER, evp_sha256,
                                    enckey.size(), enckey.data());
        if (res == 0) {
            throw std::runtime_error("Unable to hash password");
        }

        /* Encrypt backup with derived key and zero nonce */
        byte_array<crypto_stream_NONCEBYTES> nonce{};
        crypto_stream_xor(data.data() + SALT_LEN, data.data() + SALT_LEN,
                          CLIENTID_SIZE + crypto_box_SECRETKEYBYTES + HASH_LEN, nonce.data(), enckey.data());

        std::string backupString = base32_encode(data);
        for(size_t i = backupString.size(); (i -= 4) > 3;) {
            backupString.insert(i, 1, '-');
        }
        return backupString;
    }

    Account decrypt_backup(std::string const &backup_string, std::string const &password) {
        byte_array<SALT_LEN + CLIENTID_SIZE + crypto_box_SECRETKEYBYTES + HASH_LEN> backup;

        /* decode Base32 */
        if (base32_decode(backup_string, backup.data(), backup.size()) != backup.size()) {
            throw std::runtime_error("Invalid backup length");
        }

        // 0.. 7 : salt
        auto salt = slice_array<0, SALT_LEN>(backup);
        // 8 .. 49 : encrypted data
        auto enc_data = slice_array<SALT_LEN, CLIENTID_SIZE + crypto_box_SECRETKEYBYTES + 2>(backup);

        // pbkdf2+HMAC hash the password, salt with client ID. Result is encryption key
        //TODO: when there is platform support, switch to libsodium implementation
        byte_array<crypto_stream_KEYBYTES> enckey;
        const EVP_MD* evp_sha256 = EVP_sha256();
        int res = PKCS5_PBKDF2_HMAC(password.data(), password.size(), salt.data(), salt.size(), PBKDF_ITER, evp_sha256,
                                    enckey.size(), enckey.data());
        if (res == 0) {
            throw std::runtime_error("Unable to hash password");
        }

        /* Decrypt backup with derived key and zero nonce */
        byte_array<crypto_stream_NONCEBYTES> nonce{0};
        crypto_stream_xor(enc_data.data(), enc_data.data(), enc_data.size(), nonce.data(), enckey.data());

        // Backup now contains following data:
        // 8 .. 15 : client ID
        auto clientid = slice_array<SALT_LEN, CLIENTID_SIZE>(backup);
        // 16 .. 47 : private key
        auto clientsk = slice_array<SALT_LEN + CLIENTID_SIZE, crypto_box_SECRETKEYBYTES>(backup);
        // 48 .. 49 : hash prefix
        auto hash_prefix = slice_array<SALT_LEN + CLIENTID_SIZE + crypto_box_SECRETKEYBYTES, HASH_LEN>(backup);

        /* Sanity check on result by comparing truncated SHA256 hash */
        byte_array<hash_256_len> hash = hash_sha256(backup.begin() + SALT_LEN, CLIENTID_SIZE + crypto_box_SECRETKEYBYTES);

        if (!std::equal(hash_prefix.begin(), hash_prefix.end(), hash.begin())) {
            throw std::runtime_error("Backup corrupted"); // Invalid hash
        }

        client_id id;
        std::copy(clientid.begin(), clientid.end(), id.begin());
        private_key sk;
        std::copy(clientsk.begin(), clientsk.end(), sk.begin());
        return Account(id, sk);
    }
}