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

#include <protocol/packet/messagetypes.h>
#include <protocol/packet/MessageFlag.h>
#include <types/bytes.h>
#include <string>

namespace ceema {
    /**
     * Payload for text messages, contains a single string that is UTF-8 encoded
     */
    struct PayloadText {
        static constexpr MessageType Type = MessageType::TEXT;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{MessageFlag::PUSH};
        }

        /** UTF-8 encoded message text */
        std::string m_text;

        static PayloadText deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize();
    };

    /**
     * Location is sent as 3 floats (lat, long, acc) optionally followed by
     * newline and string representation
     */
    struct PayloadLocation {
        static constexpr MessageType Type = MessageType::LOCATION;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{MessageFlag::PUSH};
        }

        //TODO: This is not an accurate representation
        double m_lattitude;
        double m_longitude;
        double m_accuracy;
        std::string m_location;
        std::string m_description;

        static PayloadLocation deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize();
    };
}