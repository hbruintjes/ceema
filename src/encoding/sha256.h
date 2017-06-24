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

#pragma once

#include <cstdint>
#include <types/bytes.h>
#include <sodium/crypto_hash_sha256.h>
#include <sodium/crypto_auth_hmacsha256.h>

namespace ceema {
    const std::size_t hash_256_len = crypto_hash_sha256_BYTES;
    const std::size_t mac_256_len = crypto_auth_hmacsha256_BYTES;

    struct sha256_hash : byte_array<crypto_hash_sha256_BYTES> {
        using byte_array::byte_array;
    };

    struct hmacsha256_auth : byte_array<crypto_auth_hmacsha256_BYTES> {
        using byte_array::byte_array;
    };

    /**
     * Hash the given data using SHA256.
     * @param data Data to hash
     * @param len Length of data in bytes
     * @return Hash
     */
    sha256_hash hash_sha256(std::uint8_t const *data, std::size_t len);
    hmacsha256_auth mac_sha256(std::uint8_t const *data, std::size_t len,
                                       std::uint8_t const *key, std::size_t key_len);

}
