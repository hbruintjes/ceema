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

#include "protocol/packet/messagetypes.h"
#include <protocol/packet/payloads/PayloadControl.h>
#include <protocol/packet/payloads/PayloadText.h>
#include <protocol/packet/payloads/PayloadBlob.h>
#include <protocol/packet/payloads/PayloadPoll.h>
#include <protocol/packet/payloads/PayloadGroup.h>
#include <protocol/packet/payloads/PayloadGroupControl.h>
#include <protocol/packet/MessageFlag.h>

//#define BOOST_MPL_LIMIT_LIST_SIZE 30
//#define BOOST_VARIANT_VISITATION_UNROLLING_LIMIT 30
#include "boost/variant.hpp"

using json = nlohmann::json;

namespace ceema {

    struct PayloadNone {
        static constexpr MessageType Type = MessageType::NONE;
        static constexpr MessageFlags default_flags() {
            return MessageFlags();
        }
        byte_vector serialize();
    };

    class PayloadSerializer : public boost::static_visitor<byte_vector>
    {
    public:
        template<typename Payload>
        byte_vector operator()(Payload payload) const
        {
            return payload.serialize();
        }
    };

    class PayloadType : public boost::static_visitor<MessageType>
    {
    public:
        template<typename Payload>
        MessageType operator()(Payload) const
        {
            return Payload::Type;
        }
    };

    class PayloadFlags : public boost::static_visitor<MessageFlags>
    {
    public:
        template<typename Payload>
        MessageFlags operator()(Payload) const
        {
            return Payload::default_flags();
        }
    };

    /**
     * Class representing a union of all possible Payload types,
     * with corresponding (templated) getters.
     */
    struct MessagePayload {
        boost::variant<
                PayloadNone,

                PayloadText,
                PayloadPicture,
                PayloadLocation,
                PayloadVideo,
                PayloadAudio,
                PayloadFile,
                PayloadPoll,
                PayloadPollVote,
                PayloadMessageStatus,
                PayloadTyping,

                PayloadGroupMembers,
                PayloadGroupTitle,
                PayloadGroupIcon,
                PayloadGroupLeave,
                PayloadGroupSync,

                PayloadGroupText,
                PayloadGroupLocation,
                PayloadGroupPicture,
                //PayloadGroupVideo,
                //PayloadGroupAudio,
                PayloadGroupFile
                //PayloadGroupPoll,
                //PayloadGroupPollVote
        > m_payload;

        MessagePayload() : m_payload(PayloadNone{}) {}

        template<typename Payload>
        explicit MessagePayload(Payload const& payload) : m_payload(payload) {}

        template<typename Payload>
        auto& get() {
            return boost::get<Payload>(m_payload);
        }
        template<typename Payload>
        auto const& get() const {
            return boost::get<Payload>(m_payload);
        }

        MessageType get_type() const {
            return boost::apply_visitor( PayloadType(), m_payload );
        }

        MessageFlags default_flags() const {
            return boost::apply_visitor( PayloadFlags(), m_payload );
        }

        static MessagePayload deserialize(byte_vector const& payload_data);

        byte_vector serialize() {
            return boost::apply_visitor( PayloadSerializer(), m_payload );
        }

    };
}
