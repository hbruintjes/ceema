//
// Created by hbruintjes on 14/03/17.
//

#include "ThreeplConnection.h"

#include <libpurple/debug.h>
#include <protocol/packet/KeepAlive.h>

//Helper for packet type downcast
//TODO: creates a new deleter, which is bad
template<typename Derived, typename Base>
std::unique_ptr<Derived>
unique_ptr_cast( std::unique_ptr<Base>&& p )
{
    auto d = static_cast<Derived *>(p.release());
    return std::unique_ptr<Derived>(d);
}

bool ThreeplConnection::start_connect() {
    if (m_state != State::DISCONNECTED) {
        return false;
    }

    purple_connection_update_progress(m_connection, "Connecting", 0, 5);

    const char* host = purple_account_get_string(acct(), "server", "g-xx.0.threema.ch");
    int port = purple_account_get_int(acct(), "port", 5222);

    m_state = State::CONNECTING;
    if (purple_proxy_connect(this, acct(), host, port, &ThreeplConnection::on_connect, this) == NULL) {
        purple_connection_error_reason(m_connection, PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
                                       ("Unable to connect"));
        m_state = State::DISCONNECTED;
        return false;
    }

    //purple_connection_set_state(m_connection, PURPLE_CONNECTING);
    return true;
}

void ThreeplConnection::close() {
    if (m_state == State::DISCONNECTED) {
        return;
    }

    if (m_state == State::CONNECTING) {
        purple_proxy_connect_cancel_with_handle(this);
        m_state = State::DISCONNECTED;
        return;
    }

    m_state = State::DISCONNECTED;

    m_session.terminate();

    if (input_handler_read) {
        purple_input_remove(input_handler_read);
        input_handler_read = 0;
    }
    if (input_handler_write) {
        purple_input_remove(input_handler_write);
        input_handler_write = 0;
    }

    if (session_socket != -1) {
        ::close(session_socket);
    }
    session_socket = -1;

    //purple_connection_set_state(m_connection, PURPLE_DISCONNECTED);
}

bool ThreeplConnection::send_packet(ceema::Packet const& packet) {
    if (state() != State::CONNECTED) {
        return false;
    }
    try {
        m_session.send_packet(packet);
    } catch (ceema::socket_exception& e) {
        purple_connection_error_reason(connection(),
                                       PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
                                       e.what());
        purple_connection_set_state(m_connection, PURPLE_DISCONNECTED);
    } catch (std::exception& e) {
        purple_connection_error_reason(connection(),
                                       PURPLE_CONNECTION_ERROR_OTHER_ERROR,
                                       e.what());
        purple_connection_set_state(m_connection, PURPLE_DISCONNECTED);
    }
    return true;
}

void ThreeplConnection::send_keepalive() {
    send_packet(ceema::KeepAlive());
}

bool ThreeplConnection::send_agreement(ceema::client_id const& who, bool agree) {
    ceema::PayloadMessageStatus payload;
    payload.m_status = agree ? ceema::MessageStatus::AGREED : ceema::MessageStatus::DISAGREED;
    payload.m_ids.push_back(m_handler.lastAgreeable());

    LOG_DBG("Agree with " << who.toString() << " on msg " << m_handler.lastAgreeable());

    send_message(who, payload);

    send_packet(ceema::KeepAlive());

    return true;
}

bool ThreeplConnection::authenticate() {
    if (m_state == State::AUTHENTICATING) {
        switch(session().getState()) {
            case ceema::Session::State::DISCONNECTED:
                throw std::logic_error("Authenticating on disconnected session");
            case ceema::Session::State::WAIT_CONNECT:
                purple_connection_update_progress(connection(), "Begin handshake", 1, 5);
                break;
            case ceema::Session::State::WAIT_HELLO:
                purple_connection_update_progress(connection(), "Initiating session", 2, 5);
                break;
            case ceema::Session::State::WAIT_AUTH:
                purple_connection_update_progress(connection(), "Authenticating", 3, 5);
                break;
            default:
                purple_connection_update_progress(connection(), "Connected", 4, 5);
                purple_connection_set_state(connection(), PURPLE_CONNECTED);
                m_state = State::CONNECTED;
                break;
        }
    }
    return m_state == State::CONNECTED;
}

bool ThreeplConnection::read_packets() {
    while (session().has_packet()) {
        auto packet = m_session.get_packet();
        switch(packet->type()) {
            case ceema::PacketType::CONNECTED:
                purple_debug_info("threepl", "Connection complete\n");
                break;
            case ceema::PacketType::KEEPALIVE_ACK:
                break;
            case ceema::PacketType::ACK_SERVER:
                purple_debug_info("threepl", "TODO: Received ACK\n");
                break;
            case ceema::PacketType::MESSAGE_RECV: {
                auto msg = unique_ptr_cast<ceema::Message>(std::move(packet));
                if (msg->flags().isnset(ceema::MessageFlag::NO_ACK)) {
                    send_packet(msg->generateAck());
                }
                message_handler().onRecvMessage(std::move(msg));
                break; }
            case ceema::PacketType::DISCONNECTED:
                // This is normally the last packet on the stream, since the
                // connection is closed afterward
                // Technically not an error condition, but caused by logging in
                // somewhere else
                close();
                purple_connection_error_reason(connection(),
                                               PURPLE_CONNECTION_ERROR_NAME_IN_USE,
                                               "The account signed in somewhere else");
                break;
            default:
                purple_debug_error("threepl", "Received unknown message type %d\n", (int)packet->type());
                break;
        }
    }
    return m_state == State::CONNECTED;
}

void ThreeplConnection::on_connect(gpointer data, gint source, const gchar *error_message) {
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(data);
    ceema::Session& session = connection->m_session;

    if (source == -1) {
        if(source < 0) {
            gchar *tmp = g_strdup_printf(("Unable to connect: %s"), error_message);
            purple_connection_error_reason(connection->connection(),
                                           PURPLE_CONNECTION_ERROR_NETWORK_ERROR, tmp);
            g_free(tmp);
            //purple_connection_set_state(session->connection(), PURPLE_DISCONNECTED);
            return;
        }
    }

    bool proxy = purple_account_get_bool(connection->acct(), "proxy", TRUE) != 0;

    connection->m_state = State::AUTHENTICATING;

    connection->session_socket = source;
    session.connect(ceema::TcpClient{connection->session_socket}, proxy);

    connection->input_handler_read = purple_input_add(source, static_cast<PurpleInputCondition>(PURPLE_INPUT_READ), &on_session_data_read, data);
    connection->input_handler_write = purple_input_add(source, static_cast<PurpleInputCondition>(PURPLE_INPUT_WRITE), &on_session_data_write, data);
}

void ThreeplConnection::on_session_data_write(gpointer data, gint source, PurpleInputCondition condition) {
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(data);
    ceema::Session& session = connection->m_session;

    if (condition & PURPLE_INPUT_WRITE) {
        try {
            session.onReadyWrite();
        } catch (ceema::socket_exception& e) {
            purple_connection_error_reason(connection->connection(),
                                           PURPLE_CONNECTION_ERROR_NETWORK_ERROR, e.what());
            return;
        } catch (std::exception& e) {
            purple_connection_error_reason(connection->connection(),
                                           PURPLE_CONNECTION_ERROR_OTHER_ERROR, e.what());
            return;
        }
    }

    if (!session.hasWriteData()) {
        purple_input_remove(connection->input_handler_write);
        connection->input_handler_write = 0;
    }
}

void ThreeplConnection::on_session_data_read(gpointer data, gint source, PurpleInputCondition condition) {
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(data);
    ceema::Session& session = connection->m_session;

    bool eof = false;
    if (condition & PURPLE_INPUT_READ) {
        try {
            eof = !session.onReadyRead();
        } catch (ceema::socket_exception& e) {
            purple_connection_error_reason(connection->connection(),
                                           PURPLE_CONNECTION_ERROR_NETWORK_ERROR, e.what());
            return;
        } catch (std::exception& e) {
            connection->close();
            purple_connection_error_reason(connection->connection(),
                                           PURPLE_CONNECTION_ERROR_OTHER_ERROR, e.what());
            return;
        }
    }

    // Continue authentication if running
    if (connection->state() == State::AUTHENTICATING) {
        connection->authenticate();
    }

    // Read the packets
    if (connection->state() == State::CONNECTED) {
        try {
            connection->read_packets();
        } catch(std::exception& e) {
            connection->close();
            purple_connection_error_reason(connection->connection(),
                                           PURPLE_CONNECTION_ERROR_OTHER_ERROR, e.what());
            return;
        }
    }

    connection->connection()->last_received = time(NULL);

    if (eof && connection->state() != ThreeplConnection::State::DISCONNECTED) {
        connection->close();
        purple_connection_error_reason(connection->connection(),
                                       PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
                                       "Remote host closed the connection");
    } else {
        if (session.hasWriteData() && connection->input_handler_write == 0) {
            connection->input_handler_write = purple_input_add(source, static_cast<PurpleInputCondition>(PURPLE_INPUT_WRITE), &on_session_data_write, data);
        }
    }
}
