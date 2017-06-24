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

#include "PayloadText.h"

namespace ceema {
    PayloadText PayloadText::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size < 1) {
            throw std::runtime_error("Invalid text payload");
        }
        return PayloadText{std::string(reinterpret_cast<const char*>(&*payload_data), size)};
    }

    byte_vector PayloadText::serialize() {
        return byte_vector(m_text.begin(), m_text.end());
    }

    PayloadLocation PayloadLocation::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size < 1) {
            throw std::runtime_error("Invalid location payload");
        }
        return PayloadLocation{std::string(reinterpret_cast<const char*>(&*payload_data), size)};
    }

    byte_vector PayloadLocation::serialize() {
        return byte_vector(m_location.begin(), m_location.end());
    }
}