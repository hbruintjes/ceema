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

#include <types/bytes.h>
#include <encoding/crypto.h>

namespace ceema {
    struct message_id : public byte_array<8> {
        using byte_array::byte_array;
    };

    inline message_id gen_message_id() {
        static std::uint8_t counter = 0;
        message_id id;
        id[0] = 0xce;
        id[1] = counter++;
        randombytes_buf(id.data() + 2, id.size() - 2);
        return id;
    }
}
