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
#include <protocol/data/Blob.h>
#include <protocol/data/Group.h>
#include <types/bytes.h>
#include <string>

namespace ceema {
    struct PayloadGroupMembers {
        static constexpr MessageType Type = MessageType::GROUP_MEMBERS;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{MessageFlag::NO_ACK, MessageFlag::NO_QUEUE, MessageFlag::GROUP};
        }

        static PayloadGroupMembers deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize() const;

        group_id group;
        std::vector<client_id> members;
    };

    struct PayloadGroupTitle {
        static constexpr MessageType Type = MessageType::GROUP_TITLE;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{MessageFlag::NO_ACK, MessageFlag::NO_QUEUE, MessageFlag::GROUP};
        }

        static PayloadGroupTitle deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize() const;

        group_id group;
        std::string title;
    };

    struct PayloadGroupIcon {
        static constexpr MessageType Type = MessageType::GROUP_ICON;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{MessageFlag::NO_ACK, MessageFlag::NO_QUEUE, MessageFlag::GROUP};
        }

        static PayloadGroupIcon deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize() const;

        group_id group;

        blob_id id;
        blob_size size;
        shared_key key;
    };

    struct PayloadGroupSync {
        static constexpr MessageType Type = MessageType::GROUP_SYNC;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{MessageFlag::NO_ACK, MessageFlag::NO_QUEUE, MessageFlag::GROUP};
        }

        static PayloadGroupSync deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize() const;

        group_id group;
    };

    struct PayloadGroupLeave {
        static constexpr MessageType Type = MessageType::GROUP_LEAVE;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{MessageFlag::GROUP};
        }

        group_uid group;

        static PayloadGroupLeave deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize() const;
    };
}