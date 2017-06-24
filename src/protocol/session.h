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

#include "protocol.h"

#include "contact/Contact.h"
#include "socket/socket.h"
#include "protocol/packet/Packet.h"

#include <string>
#include <deque>
#include <contact/Account.h>

namespace ceema {
    const unsigned PROTO_NONCE_PREFIX_SIZE = 16u;
    using nonce_prefix = byte_array<PROTO_NONCE_PREFIX_SIZE>;

    /** Public key of server (fixed) */
    extern public_key const serverPK;
    extern public_key const serverPKGateway;

    inline nonce make_nonce(nonce_prefix const &prefix, std::uint64_t seq) {
        nonce n;
        auto iter = std::copy(prefix.begin(), prefix.end(), n.begin());
        htole(seq, iter);
        return n;
    }

    /**
     * Session keeps track of nonces, temporary server key and message sequence IDs
     */
    class Session {
    public:
        enum class State {
            DISCONNECTED,
            WAIT_CONNECT,
            WAIT_HELLO,
            WAIT_AUTH,
            WAIT_PKT_HEADER,
            WAIT_PKT
        };

    private:
        // Nonce information
        nonce_prefix m_noncePrefix;
        nonce_prefix m_serverNoncePrefix;
        std::uint64_t m_counter;
        std::uint64_t m_serverCounter;

        // Session (short term) keypair, public + secret
        public_key m_sessionPK;
        private_key m_sessionSK;
        public_key m_sessionServerPK;

        // Contact data
        Account m_client;

        // The connection to server
        public_key m_serverPK;
        TcpClient m_socket;
        bool m_useProxy;
        State m_state;

        // Buffers for socket data
        std::size_t m_nextReadSize;
        byte_vector m_readBuffer;
        byte_vector m_writeBuffer;

        // Packet buffer
        std::deque<std::unique_ptr<Packet>> m_packetQueue;

    public:
        Session(Account const &client);

        /**
         * Connect to remote server and perform handshake. Throws exception on error
         * @param hostname
         * @param port
         * @param serverPK
         */
        void connect(TcpClient socket, bool useProxy = true);

        void terminate();

        // Returns if not EOF
        bool onReadyRead();
        void onReadyWrite();

        bool has_packet() const;
        std::unique_ptr<Packet> get_packet();
        void send_packet(Packet const& packet);

        State getState() const {
            return m_state;
        }

        bool hasWriteData() const {
            return m_writeBuffer.size() != 0;
        }

        Account const& client() const {
            return m_client;
        }
    protected:
        /**
         * Send client hello packet (nonce prefix and session PK)
         */
        void send_hello();

        /**
         * Receive server hello packet (nonce prefix and session PK)
         */
        void read_hello();

        /**
         * Send acknowledgement and client info
         */
        void send_auth();

        /**
         * Finish handshake
         */
        void read_ack();

        nonce nextClientNonce();

        nonce nextServerNonce();

        void read_packet(std::uint16_t length);
        void send_packet(byte_vector const& data);

        void packetReady();
    };

}
