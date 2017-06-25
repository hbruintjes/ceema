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

#include "PayloadControl.h"

#include <ostream>

namespace ceema {
    PayloadMessageStatus PayloadMessageStatus::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (!size || (size-1) % message_id::array_size != 0) {
            throw std::runtime_error("Invalid status payload");
        }

        auto numIDs = (size-1) / message_id::array_size;

        PayloadMessageStatus payload;
        payload.m_status = static_cast<MessageStatus>(payload_data[0]);
        payload_data++;

        payload.m_ids.resize(numIDs);
        for(std::size_t i = 0; i < numIDs; i++) {
            std::copy(payload_data, payload_data+8, payload.m_ids[i].begin());
            payload_data += 8;
        }
        return payload;
    }

    byte_vector PayloadMessageStatus::serialize() {
        byte_vector res;
        res.resize(1 + m_ids.size()*message_id::array_size);

        res[0] = static_cast<std::uint8_t>(m_status);
        auto iter = res.begin()+1;
        for(auto const& id: m_ids) {
            iter = std::copy(id.begin(), id.end(), iter);
        }

        return res;
    }

    PayloadTyping PayloadTyping::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size != 1) {
            throw std::runtime_error("Invalid typing payload");
        }
        return PayloadTyping{!!payload_data[0]};
    }

    byte_vector PayloadTyping::serialize() {
        return byte_vector{m_typing ? std::uint8_t(1u) : std::uint8_t(0u)};
    }

    std::ostream& operator<<(std::ostream& os, MessageStatus status) {
        switch(status) {
            case MessageStatus::RECEIVED:
                return os << "RECEIVED";
            case MessageStatus::SEEN:
                return os << "SEEN";
            case MessageStatus::AGREED:
                return os << "AGREED";
            case MessageStatus::DISAGREED:
                return os << "DISAGREED";
        }
        os << "<MessageStatus 0x" << std::hex << static_cast<unsigned>(status) << ">";
        os.setstate(std::ostream::failbit);
        return os;
    }
}