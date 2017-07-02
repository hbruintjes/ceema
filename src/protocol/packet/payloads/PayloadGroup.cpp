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

#include <protocol/protocol.h>
#include <types/iter.h>
#include "PayloadGroup.h"

namespace ceema {
    PayloadGroupPicture PayloadGroupPicture::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size != group_uid::array_size + blob_id::array_size + sizeof(blob_size) + shared_key::array_size) {
            throw std::runtime_error("Invalid group picture payload");
        }

        PayloadGroupPicture payload;
        payload_data = copy_iter(payload_data, payload.group);

        payload_data = copy_iter(payload_data, payload.id);
        letoh(payload.size, &*payload_data);
        payload_data += sizeof(payload.size);
        payload_data = copy_iter(payload_data, payload.key);

        return payload;
    }

    byte_vector PayloadGroupPicture::serialize() const {
        byte_vector data;
        data.resize(group_uid::array_size + blob_id::array_size + sizeof(blob_size) + shared_key::array_size);

        auto iter = std::copy(group.begin(), group.end(), data.begin());

        iter = std::copy(id.begin(), id.end(), iter);
        htole(size, &*iter);
        iter += sizeof(blob_size);
        iter = std::copy(key.begin(), key.end(), iter);

        return data;
    }
}