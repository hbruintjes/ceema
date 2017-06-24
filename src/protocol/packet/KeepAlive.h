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

namespace ceema {

    class KeepAlive : public Packet {
        byte_vector m_payload;
    public:
        /**
         * Construct a new keep-alive packet with random payload
         */
        KeepAlive();

        byte_vector const& payload() const {
            return m_payload;
        }

        bool isAck() const {
            return m_type == PacketType ::KEEPALIVE_ACK;
        }

        KeepAlive generateAck() const;

        static KeepAlive fromPacket(PacketType type, byte_vector const& packet);

        byte_vector toPacket() const;

    private:
        /**
         * Construct Keep-alive from data
         * @param type
         * @param payload
         */
        KeepAlive(PacketType type, byte_vector const& payload) :
                Packet(type), m_payload(payload) {};

        /**
         * Construct Keep-alive without data
         * @param type
         * @param payload
         */
        KeepAlive(PacketType type) :
                Packet(type) {};
    };

}
