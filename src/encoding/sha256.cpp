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

#include "sha256.h"

#include <algorithm>
#include <logging/logging.h>

namespace ceema {

    sha256_hash hash_sha256(std::uint8_t const *data, std::size_t len) {
        sha256_hash hash;
        crypto_hash_sha256(hash.data(), data, len);
        return hash;
    }

    hmacsha256_auth mac_sha256(std::uint8_t const *data, std::size_t len,
                               std::uint8_t const *key, std::size_t key_len) {
        hmacsha256_auth hash;
        crypto_auth_hmacsha256(hash.data(), data, len, key);
        return hash;
    }

}