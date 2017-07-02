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
#include <protocol/packet/message_id.h>
#include <types/bytes.h>
#include <protocol/packet/MessageFlag.h>

#include <iosfwd>

namespace ceema {
    enum class MessageStatus : std::uint8_t {
        RECEIVED = 0x01,
        SEEN = 0x02,
        AGREED = 0x03,
        DISAGREED = 0x04,
    };

    std::ostream& operator<<(std::ostream& os, MessageStatus status);

    /**
     * Message status is a status flag and the message IDs it
     * applies to
     */
    struct PayloadMessageStatus {
        static constexpr MessageType Type = MessageType::MESSAGE_STATUS;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags();
        }
        MessageStatus m_status;
        std::vector<message_id> m_ids;

        static PayloadMessageStatus deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize() const;
    };

    /**
     * Simple boolean indicating if the user is typing a message
     */
    struct PayloadTyping {
        static constexpr MessageType Type = MessageType::CLIENT_TYPING;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{MessageFlag::NO_ACK, MessageFlag::NO_QUEUE};
        }
        bool m_typing;

        static PayloadTyping deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize() const;
    };
}