//
// Created by hbruintjes on 15/03/17.
//

#pragma once

#include <protocol/packet/Message.h>
#include <unordered_map>
#include <api/BlobAPI.h>
#include "GroupStore.h"

class ThreeplConnection;
class ContactStore;

class message_exception : public std::runtime_error {
    ceema::client_id m_cid;
public:
    message_exception(ceema::client_id id, const char* what) :
            std::runtime_error(what), m_cid(id) {
    }
    message_exception(ceema::client_id id, std::string const& what) :
            std::runtime_error(what), m_cid(id) {
    }

    ceema::client_id id() const {
        return m_cid;
    }
};

class ThreeplMessageHandler {
    ThreeplConnection& m_connection;
    ContactStore& m_contacts;
    GroupStore& m_groups;
    ceema::BlobAPI& m_blobAPI;

    std::unordered_map<ceema::client_id, std::vector<std::pair<std::unique_ptr<ceema::Message>, ceema::promise<std::unique_ptr<ceema::Message>>>>> packetQueue;

    ceema::message_id m_lastAgreeable;
public:
    ThreeplMessageHandler(ThreeplConnection& session, ContactStore& contacts, GroupStore& groups, ceema::BlobAPI& blobAPI) :
            m_connection(session), m_contacts(contacts), m_groups(groups), m_blobAPI(blobAPI),
            m_lastAgreeable{} {}

    void onRecvMessage(std::unique_ptr<ceema::Message> msg);

    template<typename Payload>
    ceema::future<std::unique_ptr<ceema::Message>> sendPayload(ceema::Account const& sender, ceema::client_id const& recipient, Payload&& payload) {
        std::unique_ptr<ceema::Message> msg = std::make_unique<ceema::Message>(
                sender.id(), recipient,
                ceema::MessagePayload(payload));
        // Threepl does not make use of ACK, so no need to request one
        // If the network is down, Purple can't do much anyway
        msg->flags().set(ceema::MessageFlag::NO_ACK);
        if (sender.nick().size()) {
            msg->nick() = sender.nick();
        }
        return sendMessage(std::move(msg));
    }

    template<typename Payload>
    std::vector<ceema::future<std::unique_ptr<ceema::Message>>> sendPayload(ceema::Account const& sender, ThreeplGroup* group, Payload&& payload) {
        std::vector<ceema::future<std::unique_ptr<ceema::Message>>> res;
        for(ceema::client_id const& member: group->members()) {
            if (member == sender.id()) {
                continue;
            }
            res.push_back(sendPayload(sender, member, payload));
        }
        return res;
    }

    ThreeplGroup* find_or_create_group(ceema::group_uid uid, bool add_chat = false);

    ceema::message_id const& lastAgreeable() const {
        return m_lastAgreeable;
    }
private:
    ceema::future<std::unique_ptr<ceema::Message>> sendMessage(std::unique_ptr<ceema::Message> msg);

    ceema::future<std::unique_ptr<ceema::Message>> enqueue(std::unique_ptr<ceema::Message> msg);
    void recv(ceema::Message& msg);
    void send(ceema::Message& msg);

};