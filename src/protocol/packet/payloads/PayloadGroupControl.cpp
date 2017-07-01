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

#include "PayloadGroupControl.h"

#include <protocol/protocol.h>
#include <logging/logging.h>
#include <types/iter.h>

namespace ceema {
    PayloadGroupMembers PayloadGroupMembers::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size < group_id::array_size || (size - group_id::array_size) % client_id::array_size != 0) {
            throw std::runtime_error("Invalid group create payload");
        }

        PayloadGroupMembers payload;
        payload_data = copy_iter(payload_data, payload.group);
        payload.members.resize((size-group_id::array_size) / client_id::array_size);
        for(auto& member: payload.members) {
            payload_data = copy_iter(payload_data, member);
        }

        return payload;
    }

    byte_vector PayloadGroupMembers::serialize() {
        byte_vector res;
        res.resize(group_id::array_size + members.size()*client_id::array_size);
        auto iter = res.begin();
        iter = std::copy(group.begin(), group.end(), iter);
        for(auto const& id: members) {
            iter = std::copy(id.begin(), id.end(), iter);
        }

        return res;
    }

    PayloadGroupTitle PayloadGroupTitle::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size < group_id::array_size) {
            throw std::runtime_error("Invalid group title payload");
        }
        auto payload_end = payload_data + size;

        PayloadGroupTitle payload;
        payload_data = copy_iter(payload_data, payload.group);
        payload.title = std::string(payload_data, payload_end);

        return payload;
    }

    byte_vector PayloadGroupTitle::serialize() {
        byte_vector res;
        res.resize(group_id::array_size + title.size());
        auto iter = res.begin();
        iter = std::copy(group.begin(), group.end(), iter);
        std::copy(title.begin(), title.end(), iter);

        return res;
    }

    PayloadGroupSync PayloadGroupSync::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size != group_id::array_size) {
            throw std::runtime_error("Invalid group sync payload");
        }

        PayloadGroupSync payload;
        copy_iter(payload_data, payload.group);

        return payload;
    }

    byte_vector PayloadGroupSync::serialize() {
        byte_vector res;
        res.resize(group_id::array_size);
        auto iter = res.begin();
        std::copy(group.begin(), group.end(), iter);

        return res;
    }

    PayloadGroupLeave PayloadGroupLeave::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size < group_uid::array_size) {
            throw std::runtime_error("Invalid group leave payload");
        }

        PayloadGroupLeave payload;
        payload_data = copy_iter(payload_data, payload.group);

        return payload;
    }

    byte_vector PayloadGroupLeave::serialize() {
        return byte_vector(group.begin(), group.end());
    }

}