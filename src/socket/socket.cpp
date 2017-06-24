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

#include "socket.h"
#include "logging/logging.h"

#ifndef _WIN32
	#include <unistd.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <fcntl.h>
#else
	#include <ws2tcpip.h>
#endif

#include <string.h>
#include <chrono>

namespace ceema {

	TcpSocket::TcpSocket(SOCKET socketid) : m_sock(socketid) {
	}

	TcpSocket::~TcpSocket() {
	}

	void TcpSocket::close() {
		if (m_sock != INVALID_SOCKET) {
#ifndef _WIN32
			::close(m_sock);
#else
			::closesocket(m_sock);
#endif
		}
		m_sock = INVALID_SOCKET;
	}

	bool TcpSocket::wait(unsigned long timeout) const {
		int fdReady = 0;
		timeval tv;
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(m_sock, &rfds);
		fdReady = ::select(m_sock + 1, &rfds, nullptr, nullptr,
						   timeout > 0 ? &tv : nullptr);

		if (fdReady == 0) {
			LOG_DEBUG(ceema::logging::loggerNetwork, "Timeout on select");
		} else if (fdReady < 0) {
			LOG_ERROR(ceema::logging::loggerNetwork,
					  "Error on select: " << format_error());
            throw socket_exception();
		}

		return fdReady >= 1;
	}

	size_t TcpSocket::send(const std::uint8_t *msg, size_t length) const {
		size_t total = 0; // how many bytes we've sent

		while (total < length) {
			int flags = 0;
#ifndef _WIN32
			flags |= MSG_NOSIGNAL;
#endif
			ssize_t n = ::send(m_sock, static_cast<const void *>(msg + total),
							   length - total, flags);
			if (n == SOCKET_ERROR) {
                if (total == 0 && would_block()) {
                    return 0;
                }
				LOG_ERROR(ceema::logging::loggerNetwork,
						  "Error sending data: " << format_error());
				throw socket_exception();
			}
			total += n;
		}
		return total;
	}

    ssize_t TcpSocket::receive(std::uint8_t *msg, size_t length, bool complete) const {
		ssize_t received = ::recv(m_sock, static_cast<void *>(msg), length,
								  complete ? MSG_WAITALL : 0);
		if (received == SOCKET_ERROR) {
            if (would_block()) {
                return -1;
            }
			LOG_ERROR(ceema::logging::loggerNetwork,
					  "Error receiving data: " << format_error());
            throw socket_exception();
		}
		return received;
	}


	bool TcpSocket::initialize() {
#ifdef WIN32
        // Init winsock (necessary for windows systems)
        WSADATA wsa;
        return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#else
		return true;
#endif
	}

	bool TcpSocket::deinitialize() {
#ifdef WIN32
        // Deinit winsock (necessary for windows systems)
        return WSACleanup() != SOCKET_ERROR;
#else
		return true;
#endif
	}

	void TcpSocket::set_blocking(bool blocking) {
#ifdef WIN32
        unsigned long mode = blocking ? 0 : 1;
        return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? true : false;
#else
		int flags = fcntl(m_sock, F_GETFL, 0);
		if (flags < 0) {
			return;
		}
		flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
		fcntl(m_sock, F_SETFL, flags);
#endif
	}


	bool TcpClient::connect_to(std::string address, std::uint16_t port) {
		LOG_TRACE(ceema::logging::loggerNetwork,
				  "Connecting to " << address << " port " << port);
		if (m_sock != INVALID_SOCKET) {
			LOG_ERROR(ceema::logging::loggerNetwork,
					  "Connect failed, socket already open");
			return false;
		}

		addrinfo hints = {0, 0, 0, 0, 0, 0, 0, 0};
#ifdef IPV6
		hints.ai_family = AF_UNSPEC;
#else
		hints.ai_family = AF_INET;
#endif
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = 0;
		hints.ai_protocol = 0;

		addrinfo *results;
		int s = getaddrinfo(address.c_str(), std::to_string(port).c_str(),
							&hints, &results);
		if (s != 0) {
			LOG_ERROR(ceema::logging::loggerNetwork,
					  "Unable to resolve address " << address << " (port "
												   << port << "): "
												   << gai_strerror(s));
			return false;
		}

		addrinfo *next_result = nullptr;
		for (next_result = results;
			 next_result != nullptr; next_result = next_result->ai_next) {
			m_sock = ::socket(next_result->ai_family,
							  next_result->ai_socktype,
							  next_result->ai_protocol);
			if (m_sock == INVALID_SOCKET) {
				LOG_DEBUG(ceema::logging::loggerNetwork,
						  "Invalid socket from getaddrinfo(): "
								  << format_error());
				continue;
			}

			char hostname[INET6_ADDRSTRLEN];
			getnameinfo(next_result->ai_addr, next_result->ai_addrlen, hostname,
						sizeof(hostname), nullptr, 0, NI_NUMERICHOST);
			LOG_DEBUG(ceema::logging::loggerNetwork,
					  "Connecting to " << hostname);
			if (::connect(m_sock, next_result->ai_addr,
						  next_result->ai_addrlen) != SOCKET_ERROR) {
				// Connect success, done
				LOG_DEBUG(ceema::logging::loggerNetwork, "Connected");
				break;
			} else {
				LOG_DEBUG(ceema::logging::loggerNetwork,
						  "Unable to connect: " << format_error());
			}
		}

		freeaddrinfo(results);

		if (next_result == nullptr) {
			LOG_ERROR(ceema::logging::loggerNetwork, "Connect failed");
			return false;
		}

		return true;
	}

}
