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
#include "encoding/crypto.h"

namespace ceema {

    struct blob_id : public byte_array<16> {
        using byte_array::byte_array;
    };

    typedef std::uint32_t blob_size;

    /**
     * Types of blobs. Mostly relevant to distinguish between the fixed
     * nonces used to encrypt/decrypt the data (except for IMAGE, which
     * has none)
     */
    enum class BlobType {
        IMAGE,
        VIDEO,
        VIDEO_THUMB,
        AUDIO,
        FILE,
        FILE_THUMB,
        GROUP_IMAGE,
        GROUP_ICON,
        ICON
    };

    /**
     * Blob as used by newer functions, based on shared key
     * and a fixed nonce
     */
    struct Blob {
        blob_id id;
        blob_size size;
        shared_key key;
    };

    /**
     * Blob used by image API, uses contact's public/private keys and
     * random nonce
     */
    struct LegacyBlob {
        blob_id id;
        blob_size size;
        nonce n;
    };
}
