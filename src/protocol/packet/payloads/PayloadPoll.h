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
#include <protocol/data/Poll.h>
#include <string>

namespace ceema {

    /**
     * Poll data is sent as JSON configuration mostly
     */
    struct PayloadPoll {
        static constexpr MessageType Type = MessageType::POLL;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{MessageFlag::PUSH};
        }

        poll_id id;

        Poll poll;

        static PayloadPoll deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize() const;
    };

    /**
     * Poll vote data is sent as JSON configuration mostly
     */
    struct PayloadPollVote {
        static constexpr MessageType Type = MessageType::POLL_VOTE;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{MessageFlag::PUSH};
        }

        poll_uid id;

        std::vector<PollVoteChoice> choices;

        static PayloadPollVote deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize() const;
    };
}
