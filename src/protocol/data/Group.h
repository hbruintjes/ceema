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

#include "Client.h"

namespace ceema {
    /**
     * Group ID used by group owner
     */
    struct group_id : public byte_array<8> {
        using byte_array::byte_array;
    };

    /**
     * Unique group ID, consisting of owner ID and group ID
     */
    struct group_uid : public byte_array<client_id::array_size + group_id::array_size> {
        using byte_array::byte_array;

        group_id gid() const {
            return group_id(slice_array<client_id::array_size, group_id::array_size>(*this));
        }

        client_id cid() const {
            return client_id(slice_array<0, client_id::array_size>(*this));
        }
    };

    /**
     * Generates a new group ID randomly
     * @return Group ID
     */
    inline group_id gen_group_id() {
        group_id id;
        randombytes_buf(id.data(), id.size());
        return id;
    }

    /**
     * Constructs a group UID from a group ID and a client ID
     * @param cid ID of the group owner
     * @param gid group ID
     * @return Group UID
     */
    inline group_uid make_group_uid(client_id const& cid, group_id const& gid) {
        group_uid uid;
        auto next = std::copy(cid.begin(), cid.end(), uid.begin());
        std::copy(gid.begin(), gid.end(), next);
        return uid;
    }
}
