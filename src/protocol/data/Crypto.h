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

#include "types/bytes.h"
#include <sodium.h>

namespace ceema {
    struct private_key : public byte_array<crypto_box_SECRETKEYBYTES> {
        using byte_array::byte_array;
    };

    struct public_key : public byte_array<crypto_box_PUBLICKEYBYTES> {
        using byte_array::byte_array;
    };

    struct shared_key : public byte_array<crypto_secretbox_KEYBYTES> {
        using byte_array::byte_array;
    };

    struct nonce : public byte_array<crypto_box_NONCEBYTES> {
        using byte_array::byte_array;
    };
}
