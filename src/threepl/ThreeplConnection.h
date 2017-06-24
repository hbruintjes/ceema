//
// Created by hbruintjes on 14/03/17.
//

#ifndef CEEMA_THREEPLCONNECTION_H
#define CEEMA_THREEPLCONNECTION_H

#include "PrplHttpManager.h"
#include "ContactStore.h"
#include "GroupStore.h"
#include "Transfer.h"
#include "MessageHandler.h"
#include <libpurple/connection.h>
#include <api/BlobAPI.h>
#include <protocol/session.h>

/**
 * Holds data association with PurpleConnection specific for Threepl
 */
class ThreeplConnection {
    enum class State {
        DISCONNECTED,
        CONNECTING,
        AUTHENTICATING,
        CONNECTED
    };

    // HTTP API
    PrplHttpManager m_httpManager;
    ceema::IdentAPI m_identAPI;
    ceema::BlobAPI m_blobAPI;

    // Messaging
    ContactStore m_store;
    GroupStore m_groups;
    ThreeplMessageHandler m_handler;

    // ceema connection data
    ceema::Session m_session;
    ceema::Account m_account;

    // Purple data
    PurpleAccount* m_prpl_acct;
    PurpleConnection* m_connection;

    gint session_socket;

    guint input_handler_read;
    guint input_handler_write;

    State m_state;

public:
    //std::unordered_map<ceema::group_uid, ThreeplGroup> m_groups;
    /** Map from ceema group to chat conv (inverse direction is handled by protocol data) */
    //std::unordered_map<ceema::group_uid, int> m_group_chats;

    //TODO: make TLS usage configurable
    ThreeplConnection(PurpleAccount* acct, ceema::Account const& account) :
            m_identAPI(m_httpManager), m_blobAPI(m_httpManager, true), m_store(m_identAPI),
            m_handler(*this, m_store, m_groups, m_blobAPI), m_session(account),
            m_account(account), m_prpl_acct(acct),
            m_connection(purple_account_get_connection(m_prpl_acct)),
            session_socket(-1), input_handler_read(0), input_handler_write(0),
            m_state(State::DISCONNECTED)
    {}

    ContactStore& contact_store() {
        return m_store;
    }

    GroupStore& group_store() {
        return m_groups;
    }

    ceema::IdentAPI& ident_API() {
        return m_identAPI;
    }

    PrplBlobUploadTransfer* new_xfer(PurpleConnection* gc, const char *who) {
        return new PrplBlobUploadTransfer(m_blobAPI, ceema::BlobType::FILE, gc, who);
    }

    ceema::Account const& account() const {
        return m_account;
    }

    const ceema::Session &session() const {
        return m_session;
    }

    PurpleAccount *acct() const {
        return m_prpl_acct;
    }

    PurpleConnection *connection() const {
        return m_connection;
    }

    State state() const {
        return m_state;
    }

    bool start_connect();

    void close();

    //TODO: the future should refernece the message being sent, not just the ID
    template<typename Payload>
    ceema::future<std::unique_ptr<ceema::Message>> send_message(ceema::client_id const& recipient, Payload&& payload) {
        if (state() != State::CONNECTED) {
            return ceema::future<std::unique_ptr<ceema::Message>>();
        }
        return m_handler.sendPayload(account(), recipient, std::move(payload));
    }

    template<typename Payload>
    std::vector<ceema::future<std::unique_ptr<ceema::Message>>> send_group_message(ThreeplGroup* group, Payload&& payload) {
        if (state() != State::CONNECTED) {
            return std::vector<ceema::future<std::unique_ptr<ceema::Message>>>();
        }
        return m_handler.sendPayload(account(), group, std::move(payload));
    }

    /**
     * Send the given packet.
     * @param packet Packet to send
     * @return true if the session is connected, false otherwise
     */
    bool send_packet(ceema::Packet const& packet);

    void send_keepalive();

    /**
     * Send agree/disagree for last received message
     * @param agree True to agree, false to disagree
     * @return true if ok, false on errors
     */
    bool send_agreement(ceema::client_id const& who, bool agree);
private:
    ThreeplMessageHandler& message_handler() {
        return m_handler;
    }

    /**
     * Continue session authentication. Returns true if completed
     * @return true iff authentication completed successfully and state is
     * connected.
     */
    bool authenticate();

    bool read_packets();

    static void on_connect(gpointer data, gint source, const gchar *error_message);

    static void on_session_data_write(gpointer data, gint source, PurpleInputCondition condition);

    static void on_session_data_read(gpointer data, gint source, PurpleInputCondition condition);
};


#endif //CEEMA_THREEPLCONNECTION_H
