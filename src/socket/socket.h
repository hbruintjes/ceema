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

#include "config.h"

#include <cstdint>
#include <stdexcept>
#include <string>

#ifndef _WIN32
	#include <sys/socket.h>
    #include <errno.h>
    #include <string.h>

    typedef int SOCKET;
	static constexpr SOCKET INVALID_SOCKET = -1;
	static constexpr int SOCKET_ERROR = -1;
#else
	#define WIN32_LEAN_AND_MEAN
	#include <winsock2.h>
    #include <Windows.h>
	static constexpr int MSG_DONTWAIT = 0; // TODO: Hack
#endif

namespace ceema {

    /**
     * Returns the last (thread local) socket error
     * @return Last socket error
     */
    inline int last_error() {
#ifndef _WIN32
        return errno;
#else
        return WSAGetLastError();
#endif
    }

    /**
     * Returns if the last error is caused by a blocking operation
     * on a non blocking socket
     * @return truee iff last operation would have blocked
     */
    inline bool would_block() {
        int err_code = last_error();
#ifndef _WIN32
        return err_code == EAGAIN || err_code == EWOULDBLOCK;
#else
        return err_code == WSAEWOULDBLOCK;
#endif
    }

    /**
     * Returns string describing error
     * @param err_code Error code to describe, defaults to last_error()
     * @return error description
     */
    inline std::string format_error(int err_code = last_error()) {
#ifndef _WIN32
        return strerror(err_code);
#else
        LPVOID lpMsgBuf;
            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL, err_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                          static_cast<LPTSTR>(&lpMsgBuf), 0, NULL);
            std::string msg{static_cast<LPTSTR>(lpMsgBuf)};
            LocalFree(lpMsgBuf);
            return msg;
#endif
    }

    /**
     * Exception occurring at socket level
     */
    class socket_exception : public std::runtime_error {
        int m_err_code;

    public:
        socket_exception(int err_code = last_error()) :
                std::runtime_error(format_error(err_code)), m_err_code(err_code) {

        }

        /**
         * Error code associated with the exception
         * @return
         */
        int err_code() const {
            return m_err_code;
        }
    };

    /**
     * Socket wrapper. Performs basic error checking
     * on socket operations. Does not actually own the socket,
     * and will not automatically close it
     */
    class TcpSocket {
    protected:
        SOCKET m_sock;

    public:
        TcpSocket(SOCKET socketid);

        TcpSocket(TcpSocket const&) = delete;
        TcpSocket(TcpSocket&&) = default;

        virtual ~TcpSocket();

        TcpSocket& operator=(TcpSocket const&) = delete;
        TcpSocket& operator=(TcpSocket&&) = default;

        /**
         * Close wrapped socket if valid
         */
        void close();

        /**
         * Send length bytes. Either sends all bytes, or if nonblocking,
         * none. Throws socket_exception if there is some
         * network error
         * @param data Data to send
         * @param length Size of the data
         * @return The number of bytes sent (=length), or 0 if
         */
        size_t send(std::uint8_t const *data, std::size_t length) const;

        /**
         * Read up to length bytes into the given buffer. Returns the number
         * of bytes read, 0 on EOF, -1 if no data is available yet and the
         * socket is nonblocking. Throws socket_exception if there is some
         * network error
         * @param data Buffer to read into
         * @param length Size of the buffer to read into
         * @param complete If true, wait until the buffer is completely filled
         * @return >0 on success, 0 on EOF, -1 if the read would block and the
         * socket is nonblocking
         */
        ssize_t receive(std::uint8_t *data, std::size_t length,
                        bool complete = false) const;

        /**
         * Wait until socket is ready for reading
         * @param timeout Time to wait until ready
         * @return true if socket ready, false if timeout expired
         */
        bool wait(unsigned long timeout) const;

        /**
         * Set the socket blocking or nonblocking
         * @param blocking Socket set blocking iff true, nonblocking
         * otherwise
         */
        void set_blocking(bool blocking);

        /**
         * Initialize socket stack on Windows
         * @return true iff init ok
         */
        static bool initialize();

        /**
         * Deinitialize socket stack on Windows
         * @return true iff deinit ok
         */
        static bool deinitialize();

    };

    /**
     * Client socket, which permits itself to be connected to
     * a (remote) host
     */
    class TcpClient : public TcpSocket {
    public:
        using TcpSocket::TcpSocket;

        /**
         * Connect to the given host and port if the socket is not yet
         * connected
         * @param address Host or IP address to connect to
         * @param port Port to connect to
         * @return true iff connection succeeded, false otherwise
         */
        bool connect_to(std::string address, std::uint16_t port);
    };

}