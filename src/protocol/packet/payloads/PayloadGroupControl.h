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
    struct PayloadGroupInfo {
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{MessageFlag::GROUP};
        }

        group_id group;
    };

    struct PayloadGroupMembers : PayloadGroupInfo {
        static constexpr MessageType Type = MessageType::GROUP_MEMBERS;

        static PayloadGroupMembers deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize() const;

        std::vector<client_id> members;
    };

    struct PayloadGroupTitle : PayloadGroupInfo {
        static constexpr MessageType Type = MessageType::GROUP_TITLE;

        static PayloadGroupTitle deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize() const;

        std::string title;
    };

    struct PayloadGroupIcon : PayloadGroupInfo {
        static constexpr MessageType Type = MessageType::GROUP_ICON;

        static PayloadGroupIcon deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize() const;

        blob_id id;
        blob_size size;
        shared_key key;
    };

    struct PayloadGroupSync : PayloadGroupInfo {
        static constexpr MessageType Type = MessageType::GROUP_SYNC;

        static PayloadGroupSync deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize() const;
    };

}