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

#include "Acknowledgement.h"

#include <protocol/protocol.h>
#include <exceptions.h>
#include <protocol/data/Client.h>
#include <types/iter.h>

namespace ceema {
    // ACK packet is fixed length
    const unsigned PAYLOAD_ACK_SIZE = sizeof(PacketType) + client_id::array_size + message_id::array_size;

    Acknowledgement Acknowledgement::fromPacket(PacketType type, byte_vector const& packet) {
        if (type != PacketType::ACK_SERVER) {
            throw protocol_exception("Invalid Acknowledgement type");
        }
        if (packet.size() != PAYLOAD_ACK_SIZE) {
            throw protocol_exception("Invalid Acknowledgement size");
        }

        Acknowledgement ack;

        auto iter = packet.begin() + sizeof(PacketType);
        copy_iter(iter, ack.m_sender);
        copy_iter(iter, ack.m_message);

        return ack;
    }

    byte_vector Acknowledgement::toPacket() const {
        byte_vector packet;
        packet.resize(PAYLOAD_ACK_SIZE);

        auto packet_iter = packet.begin();

        htole(m_type, &*packet_iter);
        packet_iter += sizeof(m_type);

        packet_iter = std::copy(m_sender.begin(), m_sender.end(), packet_iter);
        std::copy(m_message.begin(), m_message.end(), packet_iter);
        return packet;
    }

}
