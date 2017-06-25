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

#include "Message.h"

#include <encoding/crypto.h>
#include <encoding/pkcs7.h>
#include <types/iter.h>

namespace ceema {
    // MSG packet has minimum length
    const unsigned PAYLOAD_MSG_HEADER_SIZE = sizeof(PacketType) + client_id::array_size + client_id::array_size + message_id::array_size +
                                             sizeof(timestamp) + sizeof(MessageFlags) + NICKNAME_SIZE + crypto_box_NONCEBYTES +
                                             crypto_box_MACBYTES;

    Message::Message(client_id const& sender, client_id const& recipient,
                     MessagePayload const& payload) :
    Packet(PacketType::MESSAGE_SEND), m_sender(sender),
    m_recipient(recipient), m_id(gen_message_id()),
    m_time(static_cast<timestamp>(
                   std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::system_clock::now().time_since_epoch()).count())),
    m_flags(payload.default_flags()), m_nick(sender.toString()), m_nonce(crypto::generate_nonce()),
    m_message_type(payload.get_type()),
    m_payload(std::move(payload))
    {
    }

    Message::Message() : Packet(PacketType::MESSAGE_RECV), m_payload() {}

    Message Message::fromPacket(PacketType type, byte_vector const& packet) {
        if (type != PacketType::MESSAGE_RECV) {
            throw protocol_exception("Invalid Message type");
        }
        if (packet.size() < PAYLOAD_MSG_HEADER_SIZE) {
            throw protocol_exception("Invalid Message size");
        }

        LOG_TRACE(logging::loggerRoot, "Decoding message data: " << packet);

        Message m;
        auto packet_iter = packet.begin() + sizeof(PacketType);

        packet_iter = copy_iter(packet_iter, m.m_sender);

        packet_iter = copy_iter(packet_iter, m.m_recipient);

        packet_iter = copy_iter(packet_iter, m.m_id);

        letoh(m.m_time, &*packet_iter);
        packet_iter += sizeof(timestamp);

        letoh(m.m_flags.value, &*packet_iter);
        packet_iter += sizeof(MessageFlags::ValueType);

        m.m_nick = std::string(reinterpret_cast<const char*>(&*packet_iter),
                               strnlen(reinterpret_cast<const char*>(&*packet_iter), NICKNAME_SIZE));
        packet_iter += NICKNAME_SIZE;

        std::copy(packet_iter, packet_iter+crypto_box_NONCEBYTES, m.m_nonce.begin());
        packet_iter += crypto_box_NONCEBYTES;

        m.m_message_type = MessageType::NONE;

        std::size_t len = packet.size() - PAYLOAD_MSG_HEADER_SIZE + crypto_box_MACBYTES;
        m.m_payloadData.resize(len);
        packet_iter = copy_iter(packet_iter, m.m_payloadData);

        LOG_DEBUG(logging::loggerRoot, "  Sender " << m.m_sender.toString());
        LOG_DEBUG(logging::loggerRoot, "  Recipient " << m.m_recipient.toString());
        LOG_DEBUG(logging::loggerRoot, "  ID " << m.m_id);
        LOG_DEBUG(logging::loggerRoot, "  time " << m.m_time);
        LOG_DEBUG(logging::loggerRoot, "  flags " << m.m_flags);
        LOG_DEBUG(logging::loggerRoot, "  Nickname " << m.m_nick);
        LOG_DEBUG(logging::loggerRoot, "  Payload size " << len);

        return m;
    }

    byte_vector Message::toPacket() const {
        if (!m_payloadData.size()) {
            throw message_exception("Attempt to serialize unencrypted message");
        }

        byte_vector packet(PAYLOAD_MSG_HEADER_SIZE + m_payloadData.size() - crypto_box_MACBYTES);

        auto packet_iter = packet.begin();

        htole(m_type, &*packet_iter);
        packet_iter += sizeof(m_type);

        packet_iter = std::copy(m_sender.begin(), m_sender.end(), packet_iter);
        packet_iter = std::copy(m_recipient.begin(), m_recipient.end(), packet_iter);
        packet_iter = std::copy(m_id.begin(), m_id.end(), packet_iter);

        htole(m_time, &*packet_iter);
        packet_iter += sizeof(m_time);

        htole(m_flags.value, &*packet_iter);
        packet_iter += sizeof(MessageFlags::ValueType);

        if (m_nick.size() >= NICKNAME_SIZE) {
            packet_iter = std::copy(m_nick.begin(), m_nick.begin()+NICKNAME_SIZE, packet_iter);
        } else {
            auto remainder = NICKNAME_SIZE - m_nick.size();
            packet_iter = std::copy(m_nick.begin(), m_nick.end(), packet_iter);
            std::fill(packet_iter, packet_iter+remainder, 0x00);
            packet_iter += remainder;
        }

        packet_iter = std::copy(m_nonce.begin(), m_nonce.end(), packet_iter);

        packet_iter = std::copy(m_payloadData.begin(), m_payloadData.end(), packet_iter);

        if (packet_iter != packet.end()) {
            throw std::runtime_error("Error during message construction");
        }

        LOG_TRACE(logging::loggerRoot, "Message encoded as " << packet);

        return packet;
    }

    void Message::encrypt(Account const& sender, Contact const& recipient) {
        if (sender.id() != m_sender || recipient.id() != m_recipient) {
            throw std::runtime_error("Sender/Receiver mismatch");
        }

        byte_vector type_data(sizeof(MessageType));
        htole(m_message_type, type_data.data());
        byte_vector payload_data = m_payload.serialize();
        payload_data.insert(payload_data.begin(), type_data.begin(), type_data.end());
        LOG_TRACE(logging::loggerRoot, "Encrypting payload " << payload_data);
        pkcs7::add_padding(payload_data);
        payload_data.resize(payload_data.size() + crypto_box_MACBYTES);
        if (!crypto::box::encrypt_inplace(payload_data, m_nonce, recipient.pk(), sender.sk())) {
            throw std::runtime_error("Message encryption error");
        }
        m_payloadData.swap(payload_data);
    }

    void Message::decrypt(Contact const& sender, Account const& recipient) {
        if (!m_payloadData.size()) {
            throw std::runtime_error("Attempt to decrypt message without payload data");
        }
        if (sender.id() != m_sender || recipient.id() != m_recipient) {
            throw std::runtime_error("Sender/Receiver mismatch");
        }

        if (!crypto::box::decrypt_inplace(m_payloadData, m_nonce,
                                         sender.pk(), recipient.sk())) {
            throw std::runtime_error("Message decryption error");
        }

        m_payloadData.resize(m_payloadData.size() - crypto_box_MACBYTES);
        pkcs7::strip_padding(m_payloadData);

        LOG_TRACE(logging::loggerRoot, "Decrypted message data: " << m_payloadData);

        m_payload = MessagePayload::deserialize(m_payloadData);
        m_message_type = m_payload.get_type();

        LOG_TRACE(logging::loggerRoot, "Decoded message of type " << m_message_type);

        m_payloadData.clear();
    }

    std::ostream& operator<<(std::ostream& os, MessageType type) {
        switch(type) {
            case MessageType::TEXT:
                return os << "TEXT";
            case MessageType::LOCATION:
                return os << "LOCATION";
            case MessageType::ICON:
                return os << "ICON";
            case MessageType::PICTURE:
                return os << "PICTURE";
            case MessageType::VIDEO:
                return os << "VIDEO";
            case MessageType::AUDIO:
                return os << "AUDIO";
            case MessageType::POLL:
                return os << "POLL";
            case MessageType::POLL_VOTE:
                return os << "POLL_VOTE";
            case MessageType::MESSAGE_STATUS:
                return os << "MESSAGE_STATUS";
            case MessageType::CLIENT_TYPING:
                return os << "CLIENT_TYPING";
            case MessageType::FILE:
                return os << "FILE";
            case MessageType::GROUP_MEMBERS:
                return os << "GROUP_MEMBERS";
            case MessageType::GROUP_TITLE:
                return os << "GROUP_TITLE";
            case MessageType::GROUP_SYNC:
                return os << "GROUP_SYNC";
            case MessageType::GROUP_TEXT:
                return os << "GROUP_TEXT";
            case MessageType::GROUP_LOCATION:
                return os << "GROUP_LOCATION";
            case MessageType::GROUP_PICTURE:
                return os << "GROUP_PICTURE";
            case MessageType::GROUP_VIDEO:
                return os << "GROUP_VIDEO";
            case MessageType::GROUP_AUDIO:
                return os << "GROUP_AUDIO";
            case MessageType::GROUP_FILE:
                return os << "GROUP_FILE";
            case MessageType::GROUP_POLL:
                return os << "GROUP_POLL";
            case MessageType::GROUP_POLL_VOTE:
                return os << "GROUP_POLL_VOTE";
            case MessageType::GROUP_ICON:
                return os << "GROUP_ICON";
            case MessageType::GROUP_LEAVE:
                return os << "GROUP_LEAVE";
            case MessageType::NONE:
                return os << "<NONE>";
            default:
                return os << "<" << std::hex << static_cast<unsigned>(type) << ">";
        }
        throw std::domain_error("");
    }
}