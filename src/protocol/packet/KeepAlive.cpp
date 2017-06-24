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

#include "KeepAlive.h"

#include <exceptions.h>
#include <protocol/protocol.h>
#include <encoding/crypto.h>

namespace ceema {
    KeepAlive::KeepAlive() : Packet(PacketType::KEEPALIVE) {
        m_payload.resize(sizeof(std::uint32_t));
        auto val = crypto::random::random();
        *reinterpret_cast<std::uint32_t*>(m_payload.data()) = val;
    }

    KeepAlive KeepAlive::generateAck() const {
        if (isAck()) {
            throw protocol_exception("Invalid KeepAlive type to ack");
        }

        return KeepAlive(PacketType::KEEPALIVE_ACK, m_payload);
    }

    KeepAlive KeepAlive::fromPacket(PacketType type, byte_vector const& packet) {
        if (type != PacketType::KEEPALIVE && type != PacketType ::KEEPALIVE_ACK) {
            throw protocol_exception("Invalid KeepAlive type");
        }

        KeepAlive ka(type);

        ka.m_payload.resize(packet.size() - PACKET_TYPE_SIZE);
        std::copy(packet.begin() + PACKET_TYPE_SIZE, packet.end(),
                  ka.m_payload.begin());

        return ka;
    }

    byte_vector KeepAlive::toPacket() const {
        byte_vector packet;
        packet.resize(PACKET_TYPE_SIZE + m_payload.size());

        auto packet_iter = packet.begin();

        htole(m_type, &*packet_iter);
        packet_iter += sizeof(m_type);

        std::copy(m_payload.begin(), m_payload.end(), packet_iter);
        return packet;
    }
}