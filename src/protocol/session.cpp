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

#include "session.h"

#include "logging/logging.h"
#include "protocol/packet/Message.h"

#include <protocol/packet/KeepAlive.h>

#include <cstdint>
#include <cstdio>

namespace ceema {

    const unsigned PACKET_LENGTH_SIZE = sizeof(std::uint16_t);


    const unsigned PROTO_NONCE_SIZE = crypto_box_NONCEBYTES;
    const unsigned PROTO_VERSIONID_SIZE = 32u;
    const unsigned PROTO_ACK_SIZE = 16u;

    const unsigned PROTO_HELLO_PACKET_SIZE =
            PROTO_NONCE_PREFIX_SIZE + crypto_box_PUBLICKEYBYTES + PROTO_NONCE_PREFIX_SIZE +
            crypto_box_MACBYTES;
    const unsigned PROTO_AUTH_PACKET_SIZE = CLIENTID_SIZE + PROTO_VERSIONID_SIZE + PROTO_NONCE_SIZE +
                                            PROTO_NONCE_PREFIX_SIZE + crypto_box_PUBLICKEYBYTES + crypto_box_MACBYTES +
                                            crypto_box_MACBYTES;
    const unsigned PROTO_ACK_PACKET_SIZE = PROTO_ACK_SIZE + crypto_box_MACBYTES;

    Session::Session(Account const &client) :
            m_noncePrefix{0}, m_serverNoncePrefix{0}, m_counter(0), m_serverCounter(0), m_sessionPK{0}, m_sessionSK{0},
            m_sessionServerPK{0}, m_client(client), m_socket(INVALID_SOCKET), m_useProxy(false),
            m_state(State::DISCONNECTED), m_nextReadSize(0) {
    }

    void Session::connect(TcpClient socket, bool useProxy) {
        if (m_state != State::DISCONNECTED) {
            terminate();
        }
        m_socket = std::move(socket);

        m_useProxy = useProxy;
        // Store PK
        if (m_useProxy) {
            m_serverPK = serverPK;
        } else {
            m_serverPK = serverPKGateway;
        }

        // Generate nonce prefix
        crypto::randombytes(m_noncePrefix);

        // Reset the counters
        if (m_useProxy) {
            m_counter = 0u;
            m_serverCounter = 0u;
        } else {
            m_counter = 1u;
            m_serverCounter = 1u;
        }

        // Generate short-term key
        if (!crypto::generate_keypair(m_sessionPK, m_sessionSK)) {
            throw std::runtime_error("Unable to generate session keypair");
        }

        m_state = State::WAIT_CONNECT;
        packetReady();
    }

    bool Session::onReadyRead() {
        LOG_TRACE(logging::loggerSession, "onReadyRead socket");
        if (m_state == State::DISCONNECTED) {
            return false;
        }

        // Threema packets normally do not exceed 4K in size (by protocol)
        byte_array<4096> buffer;
        ssize_t recvSize;
        do {
            try {
                recvSize = m_socket.receive(buffer.data(), buffer.size(),
                                            false);
            } catch(socket_exception&) {
                terminate();
                throw;
            }
            if (recvSize > 0) {
                m_readBuffer.insert(m_readBuffer.end(), buffer.begin(),
                                    buffer.begin() + recvSize);
                LOG_TRACE(logging::loggerSession,
                          "Read " << recvSize << " bytes, expecting "
                                  << m_nextReadSize);
            }

        } while (recvSize > 0);

        while (m_nextReadSize && m_readBuffer.size() >= m_nextReadSize) {
            packetReady();
        }

        return recvSize != 0;
    }

    void Session::packetReady() {
        LOG_TRACE(logging::loggerSession, "Packet ready, state " << (unsigned)m_state);
        switch(m_state) {
            case State::WAIT_CONNECT:
                // Send HELLO packet
                send_hello();
                // Wait for HELLO packet
                m_nextReadSize = PROTO_HELLO_PACKET_SIZE;
                m_state = State::WAIT_HELLO;
                break;
            case State::WAIT_HELLO:
                // Read HELLO packet
                read_hello();
                // Send the AUTH packet
                send_auth();
                // Wait for ACK packet
                m_nextReadSize = PROTO_ACK_PACKET_SIZE;
                m_state = State::WAIT_AUTH;
                break;
            case State::WAIT_AUTH:
                // read ACK packet
                read_ack();
                // Handshake complete, begin receiving packets
                m_nextReadSize = PACKET_LENGTH_SIZE;
                m_state = State::WAIT_PKT_HEADER;
                break;
            case State::WAIT_PKT_HEADER:
                // Simply switch state to WAIT_PKT and set m_nextReadSize
                std::uint16_t length;
                letoh(length, m_readBuffer.data());
                m_readBuffer.erase(m_readBuffer.begin(), m_readBuffer.begin()+PACKET_LENGTH_SIZE);
                m_nextReadSize = length;
                m_state = State::WAIT_PKT;
                break;
            case State::WAIT_PKT:
                // Fetch packet
                read_packet(static_cast<std::uint16_t>(m_nextReadSize));
                // Wait for the next one
                m_nextReadSize = PACKET_LENGTH_SIZE;
                m_state = State::WAIT_PKT_HEADER;
                break;
            case State::DISCONNECTED:
                // WUT?
                break;
        }
        LOG_TRACE(logging::loggerSession, "Packet handled, state " << (unsigned)m_state << ", remaining data " << m_readBuffer.size());
    }

    void Session::onReadyWrite() {
        LOG_TRACE(logging::loggerSession, "onReadyWrite " << m_writeBuffer.size());
        while (m_writeBuffer.size()) {
            // TODO: Send data in chunks of 4K
            auto sendSize = m_writeBuffer.size();
            try {
                if (m_socket.send(m_writeBuffer.data(), sendSize) == sendSize) {
                    LOG_TRACE(logging::loggerSession, "Written " << sendSize << " bytes");
                    m_writeBuffer.erase(m_writeBuffer.begin(), m_writeBuffer.begin()+sendSize);
                } else {
                    // Socket became not-ready
                    break;
                }
            } catch(socket_exception&) {
                terminate();
                throw;
            }
        }
    }

    void Session::terminate() {
        // Session does not own the socket FD, simply clear it
        m_socket = TcpClient(INVALID_SOCKET);

        // Wipe data
        m_noncePrefix.fill(0);
        m_sessionPK.fill(0);
        m_sessionSK.fill(0);

        m_state = State::DISCONNECTED;
        m_nextReadSize = 0;
        m_readBuffer.clear();
        m_writeBuffer.clear();
    }

    nonce Session::nextClientNonce() {
        return make_nonce(m_noncePrefix, m_counter++);
    }

    nonce Session::nextServerNonce() {
        return make_nonce(m_serverNoncePrefix, m_serverCounter++);
    }

    void Session::send_hello() {
        LOG_TRACE(logging::loggerSession, "Sending session PK: " << m_sessionPK);
        m_writeBuffer.insert(m_writeBuffer.end(), m_sessionPK.begin(), m_sessionPK.end());
        LOG_TRACE(logging::loggerSession, "Sending nonce: " << m_noncePrefix);
        m_writeBuffer.insert(m_writeBuffer.end(), m_noncePrefix.begin(), m_noncePrefix.end());

        onReadyWrite();
    }

    void Session::read_hello() {
        byte_array<PROTO_HELLO_PACKET_SIZE> packet{m_readBuffer};
        m_readBuffer.erase(m_readBuffer.begin(), m_readBuffer.begin() + PROTO_HELLO_PACKET_SIZE);

        auto prefix = slice_array<0, PROTO_NONCE_PREFIX_SIZE>(packet);
        auto crypto = slice_array<PROTO_NONCE_PREFIX_SIZE, PROTO_HELLO_PACKET_SIZE-PROTO_NONCE_PREFIX_SIZE>(packet);

        std::copy(prefix.begin(), prefix.end(), m_serverNoncePrefix.begin());
        LOG_TRACE(logging::loggerSession, "Got server nonce: " << m_serverNoncePrefix);

        // Cyphertext in last 64 bytes of packet
        if (crypto::box::decrypt_inplace(crypto, nextServerNonce(), m_serverPK, m_sessionSK)) {
            auto serverSPK = slice_array<0, crypto_box_PUBLICKEYBYTES>(crypto);
            auto client_prefix = slice_array<crypto_box_PUBLICKEYBYTES, PROTO_NONCE_PREFIX_SIZE>(crypto);
            // Store the short term public key
            std::copy(serverSPK.begin(), serverSPK.end(), m_sessionServerPK.begin());
            LOG_TRACE(logging::loggerSession, "Got server session PK: " << m_sessionServerPK);

            // Verify client nonce correct
            if (!std::equal(m_noncePrefix.begin(), m_noncePrefix.end(), client_prefix.begin())) {
                throw session_exception("Invalid client nonce in response");
            } else {
                LOG_TRACE(logging::loggerSession, "Server HELLO OK");
            }

        } else {
            throw session_exception("Unable to decrypt HELLO packet");
        }
    }

    void Session::send_auth() {
        // Zero initialize to avoid garbage in version ID
        byte_array<PROTO_AUTH_PACKET_SIZE> packet{};

        auto packet_iter = packet.begin();
        // Username
        packet_iter = std::copy(m_client.id().begin(), m_client.id().end(), packet_iter);

        // Sys info
        const char *ver_info = "libceema 0.0.1;;;";
        strncpy((char *) &*packet_iter, ver_info, PROTO_VERSIONID_SIZE);
        packet_iter += PROTO_VERSIONID_SIZE;
        LOG_TRACE(logging::loggerSession, "Sending version " << ver_info);

        // Random nonce
        nonce ran_nonce = crypto::generate_nonce();

        if (m_useProxy) {
            packet_iter = std::copy(ran_nonce.begin(), ran_nonce.end(), packet_iter);
            packet_iter = std::copy(m_serverNoncePrefix.begin(), m_serverNoncePrefix.end(), packet_iter);
        } else {
            packet_iter = std::copy(m_serverNoncePrefix.begin(), m_serverNoncePrefix.end(), packet_iter);
            packet_iter = std::copy(ran_nonce.begin(), ran_nonce.end(), packet_iter);
        }

        // Encrypted public key
        byte_array<crypto_box_PUBLICKEYBYTES + crypto_box_MACBYTES> crypto{};
        if (!crypto::box::encrypt(crypto, m_sessionPK, ran_nonce, m_serverPK, m_client.sk())) {
            throw std::runtime_error("Error encrypting AUTH secret");
        }
        packet_iter = std::copy(crypto.begin(), crypto.end(), packet_iter);

        // Full packet encrypted
        if (!crypto::box::encrypt_inplace(packet, nextClientNonce(), m_sessionServerPK, m_sessionSK)) {
            throw std::runtime_error("Error encrypting AUTH packet");
        }

        m_writeBuffer.insert(m_writeBuffer.end(), packet.begin(), packet.end());
        onReadyWrite();
    }

    void Session::read_ack() {
        byte_array<PROTO_ACK_PACKET_SIZE> packet{m_readBuffer};
        m_readBuffer.erase(m_readBuffer.begin(), m_readBuffer.begin() + PROTO_ACK_PACKET_SIZE);

        if (!crypto::box::decrypt_inplace(packet, nextServerNonce(), m_sessionServerPK, m_sessionSK)) {
            LOG_TRACE(logging::loggerSession, "Ack decrypt failed");
            throw session_exception("Unable to decrypt ACK packet");
        }

        byte_array<PROTO_ACK_SIZE> expected_ack{0};
        if (!std::equal(expected_ack.begin(), expected_ack.end(), packet.begin())) {
            throw session_exception("Invalid ACK");
        }
        LOG_TRACE(logging::loggerSession, "Server ACK OK");
    }

    bool Session::has_packet() const {
        return !m_packetQueue.empty();
    }
    std::unique_ptr<Packet> Session::get_packet() {
        if (m_packetQueue.empty()) {
            return nullptr;
        }
        auto pkt = std::move(m_packetQueue.front());
        m_packetQueue.pop_front();
        return pkt;
    }

    void Session::send_packet(Packet const& packet) {
        switch(packet.type()) {
            case PacketType::ACK_CLIENT:
                send_packet(static_cast<Acknowledgement const&>(packet).toPacket());
                break;
            case PacketType::MESSAGE_SEND:
                send_packet(static_cast<Message const&>(packet).toPacket());
                break;
            case PacketType::KEEPALIVE:
            case PacketType::KEEPALIVE_ACK:
                send_packet(static_cast<KeepAlive const&>(packet).toPacket());
                break;
            case PacketType::CONNECTED:
            case PacketType::DISCONNECTED:
            case PacketType::MESSAGE_RECV:
            case PacketType::ACK_SERVER:
                throw protocol_exception("Attempt to send invalid packet type");
        }
    }

    void Session::read_packet(std::uint16_t length) {
        // Copy body data
        byte_vector body(m_readBuffer.begin(), m_readBuffer.begin() + length);
        m_readBuffer.erase(m_readBuffer.begin(), m_readBuffer.begin() + length);

        if (length < crypto_box_MACBYTES) {
            throw session_exception("Invalid packet size");
        }

        // Decrypt it
        if (!crypto::box::decrypt_inplace(body, nextServerNonce(), m_sessionServerPK, m_sessionSK)) {
            throw session_exception("Failed to decrypt message");
        }

        // Truncate MAC
        body.resize(body.size() - crypto_box_MACBYTES);

        try
        {
            m_packetQueue.emplace_back( Packet::fromPacket(body) );
        }
        catch (const protocol_exception& e)
        {
            std::uintmax_t packet_type;
            if (1 == std::sscanf(
                e.what(), "Unexpect packet type: 0x%jx", &packet_type))
            {
                switch (packet_type)
                {
                case 0x209:
                    LOG_TRACE(logging::loggerSession, e.what());
                    return;
                }
            }
            throw;
        }
        LOG_TRACE(logging::loggerSession, "Got packet of type " << m_packetQueue.back()->type());
    }

    void Session::send_packet(byte_vector const& data) {
        byte_vector packet(PACKET_LENGTH_SIZE + data.size() + crypto_box_MACBYTES);

        htole(data.size() + crypto_box_MACBYTES, packet.data());
        auto iter = packet.begin() + PACKET_LENGTH_SIZE;

        //TODO: need crypto:: interface when encrypting with an offset
        int res = crypto_box_easy(&*iter, data.data(), data.size(), nextClientNonce().data(), m_sessionServerPK.data(),
                                  m_sessionSK.data());
        if (res != 0) {
            throw std::runtime_error("Failed to encrypt packet body");
        }

        m_writeBuffer.insert(m_writeBuffer.end(), packet.begin(), packet.end());
        onReadyWrite();
    }

    public_key const serverPK{
            0xb8, 0x51, 0xae, 0x1b, 0xf2, 0x75, 0xeb, 0xe6,
            0x85, 0x1c, 0xa7, 0xf5, 0x20, 0x6b, 0x49, 0x50,
            0x80, 0x92, 0x71, 0x59, 0x78, 0x7e, 0x9a, 0xaa,
            0xbb, 0xeb, 0x4e, 0x55, 0xaf, 0x09, 0xd8, 0x05 };

    public_key const serverPKGateway{
            0x45, 0x0b, 0x97, 0x57, 0x35, 0x27, 0x9f, 0xde,
            0xcb, 0x33, 0x13, 0x64, 0x8f, 0x5f, 0xc6, 0xee,
            0x9f, 0xf4, 0x36, 0x0e, 0xa9, 0x2a, 0x8c, 0x17,
            0x51, 0xc6, 0x61, 0xe4, 0xc0, 0xd8, 0xc9, 0x09 };

}
