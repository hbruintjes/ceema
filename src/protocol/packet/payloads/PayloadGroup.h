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

#include <protocol/packet/payloads/PayloadText.h>
#include <protocol/packet/payloads/PayloadBlob.h>
#include <protocol/packet/payloads/PayloadPoll.h>
#include <protocol/data/Group.h>

#include <string>

namespace ceema {

    template<typename Payload>
    struct PayloadGroupBase : public Payload {
        group_uid group;

        static /*constexpr*/ MessageFlags default_flags() {
            auto flags = Payload::default_flags();
            return flags | MessageFlag::GROUP;
        }

        static PayloadGroupBase deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
            if (size < group_uid::array_size) {
                throw std::runtime_error("Invalid group payload");
            }

            PayloadGroupBase<Payload> payload;
            payload_data = copy_iter(payload_data, payload.group);
            size -= group_uid::array_size;
            static_cast<Payload&>(payload) = Payload::deserialize(payload_data, size);

            return payload;
        }

        byte_vector serialize() const {
            byte_vector data = Payload::serialize();
            data.insert(data.begin(), group.begin(), group.end());
            return data;
        }
    };

    struct PayloadGroupText : public PayloadGroupBase<PayloadText> {
        static constexpr MessageType Type = MessageType::GROUP_TEXT;
    };

    struct PayloadGroupLocation : public PayloadGroupBase<PayloadLocation>  {
        static constexpr MessageType Type = MessageType::GROUP_LOCATION;
    };

    struct PayloadGroupPicture {
        static constexpr MessageType Type = MessageType::GROUP_PICTURE;

        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{MessageFlag::PUSH, MessageFlag::GROUP};
        }

        group_uid group;

        blob_id id;
        blob_size size;
        shared_key key;

        static PayloadGroupPicture deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize() const;
    };

    struct PayloadGroupVideo : public PayloadGroupBase<PayloadVideo>  {
        static constexpr MessageType Type = MessageType::GROUP_VIDEO;
    };

    struct PayloadGroupAudio : public PayloadGroupBase<PayloadAudio> {
        static constexpr MessageType Type = MessageType::GROUP_AUDIO;
    };

    struct PayloadGroupFile : public PayloadGroupBase<PayloadFile> {
        static constexpr MessageType Type = MessageType::GROUP_FILE;
    };

    struct PayloadGroupPoll : public PayloadGroupBase<PayloadPoll> {
        static constexpr MessageType Type = MessageType::GROUP_POLL;
    };

    struct PayloadGroupPollVote : public PayloadGroupBase<PayloadPollVote> {
        static constexpr MessageType Type = MessageType::GROUP_POLL_VOTE;
    };

}