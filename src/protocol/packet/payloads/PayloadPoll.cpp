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

#include "PayloadPoll.h"

namespace ceema {

    PayloadPoll PayloadPoll::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size < poll_id::array_size) {
            throw std::runtime_error("Invalid poll payload");
        }
        auto end_data = payload_data + size;

        PayloadPoll poll;
        std::copy(payload_data, payload_data + poll_id::array_size, poll.id.begin());
        payload_data += poll_id::array_size;

        json val = json::parse(payload_data, end_data);
        poll.poll = val.get<Poll>();

        return poll;

    }

    byte_vector PayloadPoll::serialize() const {
        byte_vector res;
        res.insert(res.begin(), id.begin(), id.end());
        json poll = json(this->poll);
        std::string jsonString = poll.dump();
        res.insert(res.end(), jsonString.begin(), jsonString.end());

        return res;
    }

    PayloadPollVote PayloadPollVote::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size < poll_uid::array_size) {
            throw std::runtime_error("Invalid poll vote payload");
        }
        auto end_data = payload_data + size;

        PayloadPollVote payload;
        std::copy(payload_data, payload_data + poll_uid::array_size, payload.id.begin());
        payload_data += poll_uid::array_size;

        json val = json::parse(payload_data, end_data);
        payload.choices = val.get<std::vector<PollVoteChoice>>();

        return payload;
    }

    byte_vector PayloadPollVote::serialize() const {
        byte_vector res;
        res.insert(res.begin(), id.begin(), id.end());
        json choices = json(this->choices);
        std::string jsonString = choices.dump();
        res.insert(res.end(), jsonString.begin(), jsonString.end());

        return res;
    }

}