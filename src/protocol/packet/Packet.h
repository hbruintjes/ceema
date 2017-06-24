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

#include "types/bytes.h"

#include <cstdint>
#include <memory>
#include <iosfwd>

namespace ceema {

    const unsigned PACKET_TYPE_SIZE = 4;

    enum class PacketType : std::uint32_t {
        KEEPALIVE = 0x00,

        MESSAGE_SEND = 0x01,
        MESSAGE_RECV = 0x02,

        KEEPALIVE_ACK = 0x80,

        ACK_SERVER = 0x81,
        ACK_CLIENT = 0x82,

        CONNECTED = 0xd0,
        DISCONNECTED = 0xe0,
    };

    /**
     * Packets consist of a 4-byte type and some payload.
     * The payload is defined by sub-classes.
     */
    class Packet {
    protected:
        PacketType m_type;
    public:
        PacketType type() const {
            return m_type;
        }

        /**
         * Return pointer to a new packet based on the received packet data
         * (not including the leading protocol length
         * bytes).
         * @param data
         * @return
         */
        static std::unique_ptr<Packet> fromPacket(byte_vector const& data);

    protected:
        Packet(PacketType type) : m_type(type) {};
    };

    std::ostream& operator<<(std::ostream& os, PacketType type);
}
