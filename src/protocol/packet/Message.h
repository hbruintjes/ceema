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

#include <logging/logging.h>

#include "types/bytes.h"
#include "protocol/protocol.h"
#include "Packet.h"
#include "messagetypes.h"
#include "message_id.h"
#include "contact/Contact.h"
#include "protocol/packet/payloads/MessagePayload.h"
#include "Acknowledgement.h"
#include "MessageFlag.h"
#include <contact/Account.h>
#include <types/flags.h>

#include <exceptions.h>
#include <cassert>
#include <string>
#include <string.h>

namespace ceema {
    typedef std::uint32_t timestamp;

    const unsigned NICKNAME_SIZE = 32u;

    class Message : public Packet {
        client_id m_sender;
        client_id m_recipient;

        message_id m_id;

        timestamp m_time;

        MessageFlags m_flags;

        std::string m_nick;

        nonce m_nonce;

        byte_vector m_payloadData;

        MessagePayload m_payload;

    public:
        template<typename Payload>
        Message(client_id const& sender, client_id const& recipient,
                Payload && payload) :
                Packet(PacketType::MESSAGE_SEND), m_sender(sender),
                m_recipient(recipient), m_id(gen_message_id()),
                m_time(static_cast<timestamp>(
                               std::chrono::duration_cast<std::chrono::seconds>(
                                       std::chrono::system_clock::now().time_since_epoch()).count())),
                m_flags(payload.default_flags()), m_nick(sender.toString()), m_nonce(crypto::generate_nonce()),
                m_payload(std::forward<Payload>(payload))
        {
        }

        client_id const& sender() const {
            return m_sender;
        }
        client_id const& recipient() const {
            return m_recipient;
        }

        message_id const& id() const {
            return m_id;
        }

        timestamp const& time() const {
            return m_time;
        }
        MessageFlags const& flags() const {
            return m_flags;
        }
        MessageFlags& flags() {
            return m_flags;
        }

        std::string const& nick() const {
            return m_nick;
        }

        std::string& nick() {
            return m_nick;
        }

        nonce const& data_nonce() const {
            return m_nonce;
        }

        MessageType payloadType() const {
            return m_payload.get_type();
        }

        template<typename Payload>
        auto& payload() {
            return m_payload.get<Payload>();
        }

        template<typename Payload>
        auto const& payload() const {
            return m_payload.get<Payload>();
        }

        Acknowledgement generateAck() const {
            return Acknowledgement(sender(), id());
        }

        static Message fromPacket(PacketType type, byte_vector const& packet);

        byte_vector toPacket() const;

        void encrypt(Account const& sender, Contact const& recipient);

        void decrypt(Contact const& sender, Account const& recipient);

    private:
        Message();
    };




}
