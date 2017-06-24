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

#include "Packet.h"
#include "message_id.h"
#include <protocol/data/Client.h>

namespace ceema {

    class Acknowledgement : public Packet {
        client_id m_sender;
        message_id m_message;

    public:
        // Creates ACK packet of type ACK_CLIENT (see fromPacket for ACK_SERVER)
        Acknowledgement(client_id const& sender, message_id const& message) : Packet(PacketType::ACK_CLIENT),
                                                                              m_sender(sender), m_message(message) {}
        Acknowledgement(Acknowledgement const&) = default;
        Acknowledgement(Acknowledgement &&) = default;

        Acknowledgement& operator=(Acknowledgement const&) = default;
        Acknowledgement& operator=(Acknowledgement &&) = default;

        client_id const& sender() const {
            return m_sender;
        }

        message_id const& message() const {
            return m_message;
        }

        static Acknowledgement fromPacket(PacketType type, byte_vector const& packet);
        byte_vector toPacket() const;

    private:
        Acknowledgement() : Packet(PacketType::ACK_SERVER) {}

    };

}
