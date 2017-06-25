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

#include "Packet.h"

#include "Status.h"
#include "Message.h"
#include "Acknowledgement.h"
#include "KeepAlive.h"

#include "logging/logging.h"
#include <types/formatstr.h>

namespace ceema {

    std::unique_ptr<Packet> Packet::fromPacket(byte_vector const &data) {
        PacketType type;
        letoh(type, data.data());

        switch (type) {
            case PacketType::CONNECTED:
            case PacketType::DISCONNECTED:
                // Status Packet
                return std::make_unique<Status>(type);
            case PacketType::ACK_SERVER:
                return std::make_unique<Acknowledgement>(
                        Acknowledgement::fromPacket(type, data));
            case PacketType::MESSAGE_RECV:
                return std::make_unique<Message>(
                        Message::fromPacket(type, data));
            case PacketType::KEEPALIVE:
            case PacketType::KEEPALIVE_ACK:
                return std::make_unique<KeepAlive>(
                        KeepAlive::fromPacket(type, data));
            default:
                throw protocol_exception(formatstr() << "Unexpect packet type: 0x" <<
                                         std::hex << std::to_string(static_cast<unsigned>(type)));
        }
    }

    std::ostream& operator<<(std::ostream& os, PacketType type) {
        switch (type) {
            case PacketType::CONNECTED:
                return os << "CONNECTED";
            case PacketType::DISCONNECTED:
                return os << "DISCONNECTED";
            case PacketType::ACK_SERVER:
                return os << "ACK_SERVER";
            case PacketType::ACK_CLIENT:
                return os << "ACK_CLIENT";
            case PacketType::MESSAGE_RECV:
                return os << "MESSAGE_RECV";
            case PacketType::MESSAGE_SEND:
                return os << "MESSAGE_SEND";
            case PacketType::KEEPALIVE:
                return os << "KEEPALIVE";
            case PacketType::KEEPALIVE_ACK:
                return os << "KEEPALIVE_ACK";
        }
        os << "<PacketType 0x" << std::hex << static_cast<unsigned>(type) << ">";
        os.setstate(std::ostream::failbit);
        return os;
    }

}
