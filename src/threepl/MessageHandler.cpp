//
// Created by hbruintjes on 15/03/17.
//

#include <libpurple/debug.h>
#include <api/BlobTransfer.h>
#include "MessageHandler.h"

#include "ThreeplConnection.h"
#include "ContactStore.h"
#include "Transfer.h"

void ThreeplMessageHandler::onRecvMessage(std::unique_ptr<ceema::Message> msg) {
    if (m_contacts.has_contact(msg->sender())) {
        recv(*msg);
    } else {
        enqueue(std::move(msg));
    }
}

ceema::future<std::unique_ptr<ceema::Message>> ThreeplMessageHandler::sendMessage(std::unique_ptr<ceema::Message> msg) {
    if (m_contacts.has_contact(msg->recipient())) {
        ceema::promise<std::unique_ptr<ceema::Message>> prom;
        try {
            send(*msg);
            prom.set_value(std::move(msg));
        } catch (std::exception&) {
            prom.set_exception(std::current_exception());
        }
        return prom.get_future();
    } else {
        return enqueue(std::move(msg));
    }
}

ThreeplGroup* ThreeplMessageHandler::find_or_create_group(ceema::group_uid uid, bool add_chat) {
    ThreeplGroup* group = m_groups.find_group(uid, false);
    if (!group) {
        group = m_groups.add_group(uid);
        // Request sync
        sendPayload(m_connection.account(), group->owner(), ceema::PayloadGroupSync{});
        if (add_chat) {
            serv_got_joined_chat(m_connection.connection(), group->id(), group->name().c_str());
            m_groups.update_chat(m_connection.acct(), *group);

            GHashTable* components = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
            g_hash_table_insert(components, g_strdup("id"), g_strdup(ceema::hex_encode(group->gid()).c_str()));
            g_hash_table_insert(components, g_strdup("owner"), g_strdup(group->owner().toString().c_str()));
            auto chat = purple_chat_new(m_connection.acct(), group->name().c_str(), components);
            purple_blist_add_chat(chat, NULL, NULL);
        }
    }
    return group;
}

ceema::future<std::unique_ptr<ceema::Message>> ThreeplMessageHandler::enqueue(std::unique_ptr<ceema::Message> msg) {
    // Fetch contact, queue message
    ceema::client_id contact_id = (msg->type() == ceema::PacketType::MESSAGE_RECV) ? msg->sender() : msg->recipient();
    auto prom = ceema::promise<std::unique_ptr<ceema::Message>>();
    auto queue_fut = prom.get_future();
    auto iter = packetQueue.find(contact_id);
    if (iter != packetQueue.end()) {
        iter->second.emplace_back(std::move(msg), std::move(prom));
    } else {
        packetQueue.emplace(contact_id, std::vector<std::pair<std::unique_ptr<ceema::Message>, ceema::promise<std::unique_ptr<ceema::Message>>>>{});
        packetQueue[contact_id].emplace_back(std::move(msg), std::move(prom));
        m_contacts.fetch_contact(contact_id).next([this, contact_id](ceema::future<ContactPtr> fut) {
            try {
                ContactPtr contact = fut.get();
                for(auto& queue_iter: packetQueue[contact_id]) {
                    if (queue_iter.first->type() == ceema::PacketType::MESSAGE_RECV) {
                        this->recv(*queue_iter.first);
                    } else {
                        this->send(*queue_iter.first);
                    }
                    queue_iter.second.set_value(std::move(queue_iter.first));
                }
            } catch(std::exception& e) {
                for(auto& queue_iter: packetQueue[contact_id]) {
                    queue_iter.second.set_exception(std::make_exception_ptr(message_exception(contact_id, e.what())));
                }
            }
            packetQueue.erase(contact_id);

            return true;
        });
    }
    return queue_fut;
}

void ThreeplMessageHandler::recv(ceema::Message& msg) {
    std::string sender = msg.sender().toString();

    if (msg.nick().size()) {
        serv_got_alias(m_connection.connection(), sender.c_str(), msg.nick().c_str());
        // Check if there is a buddy for this contact
        // If so, update nickname from message
        PurpleBuddy *buddy = purple_find_buddy(m_connection.acct(), sender.c_str());
        if (buddy) {
            purple_blist_node_set_string(&buddy->node, "nickname", msg.nick().c_str());
        }
    }

    try {
        auto contactptr = m_contacts.get_contact(msg.sender());
        if (!contactptr) {
            throw std::runtime_error("Unable to fetch contact key");
        }
        msg.decrypt(*contactptr, m_connection.account());
    } catch (std::exception& e) {
        purple_conv_present_error(msg.sender().toString().c_str(), m_connection.acct(), "Unable to decrypt incoming message:");
        purple_conv_present_error(msg.sender().toString().c_str(), m_connection.acct(), e.what());
        // As potentially a lot of this can be generated, do not create a notification
        // dialog when no conversation exists
        // Silently fail...
    }
    switch(msg.payloadType()) {
        case ceema::MessageType::MESSAGE_STATUS: {
            ceema::PayloadMessageStatus const& payload = msg.payload<ceema::PayloadMessageStatus>();
            LOG_DBG("TODO: Got message status " << payload.m_status);
            break; }
        case ceema::MessageType::CLIENT_TYPING: {
            ceema::PayloadTyping const& payload = msg.payload<ceema::PayloadTyping>();
            if (payload.m_typing) {
                serv_got_typing(m_connection.connection(), sender.c_str(), 0, PURPLE_TYPING);
            } else {
                serv_got_typing_stopped(m_connection.connection(), sender.c_str());
            }
            break; }
        case ceema::MessageType::TEXT: {
            m_lastAgreeable = msg.id();

            ceema::PayloadText const& payload = msg.payload<ceema::PayloadText>();
            serv_got_im(m_connection.connection(), sender.c_str(),
                        payload.m_text.c_str(),
                        PURPLE_MESSAGE_RECV, msg.time());
            break; }
        case ceema::MessageType::LOCATION: {
            m_lastAgreeable = msg.id();

            ceema::PayloadLocation const& payload = msg.payload<ceema::PayloadLocation>();
            serv_got_im(m_connection.connection(), sender.c_str(),
                        payload.m_location.c_str(),
                        PURPLE_MESSAGE_RECV, msg.time());
            break; }
        case ceema::MessageType::FILE: {
            ceema::PayloadFile const& payload = msg.payload<ceema::PayloadFile>();

            ceema::Blob fileBlob{payload.id, payload.size, payload.key};
            PrplBlobDownloadTransfer* transfer = new PrplBlobDownloadTransfer(m_blobAPI, fileBlob, ceema::BlobType::FILE,
                                                                      m_connection.connection(), sender.c_str());
            transfer->get_future().next([this, id{payload.id}](ceema::future<void> fut) {
                m_blobAPI.deleteBlob(id);
                fut.get();
            });

            purple_xfer_set_filename(transfer->xfer(), payload.filename.c_str());
            if (payload.caption.size()) {
                purple_xfer_set_message(transfer->xfer(), payload.caption.c_str());
            }

            if (payload.has_thumb) {
                auto thumbTransfer = new ceema::BlobDownloadTransfer(payload.thumb_id, ceema::BlobType::FILE_THUMB, payload.key);
                m_blobAPI.downloadFile(thumbTransfer, payload.thumb_id);
                thumbTransfer->get_future().next([this, transfer, id{payload.thumb_id}, sender{msg.sender()}](ceema::future<ceema::byte_vector> fut) {
                    m_blobAPI.deleteBlob(id);
                    try {
                        ceema::byte_vector data = fut.get();
                        purple_xfer_set_thumbnail(transfer->xfer(), data.data(), data.size(), "jpg");
                        LOG_DBG("Got thumbnail data");
                    } catch (std::exception& e) {
                        LOG_DBG("Thumbnail error " << e.what());
                    }

                    purple_xfer_request(transfer->xfer());
                });
            } else {
                purple_xfer_request(transfer->xfer());
            }
            break; }
        case ceema::MessageType::GROUP_SYNC: {
            ceema::PayloadGroupSync const& payload = msg.payload<ceema::PayloadGroupSync>();
            ThreeplGroup* group = m_groups.find_group(msg.sender(), payload.group);
            if (group) {
                ceema::PayloadGroupMembers payloadMembers;
                payloadMembers.group = group->gid();
                payloadMembers.members = group->members();

                ceema::PayloadGroupTitle payloadTitle;
                payloadTitle.group = group->gid();
                payloadTitle.title = group->name();

                sendPayload(m_connection.account(), msg.sender(), payloadMembers);
                sendPayload(m_connection.account(), msg.sender(), payloadTitle);
            }
            break; }
        case ceema::MessageType::GROUP_TITLE: {
            ceema::PayloadGroupTitle const& payload = msg.payload<ceema::PayloadGroupTitle>();
            ThreeplGroup* group = m_groups.find_group(msg.sender(), payload.group);
            if (group) {
                group->set_name(payload.title);
                m_groups.update_chat(m_connection.acct(), *group);
            }
            break;
        }
        case ceema::MessageType::GROUP_MEMBERS: {
            ceema::PayloadGroupMembers const& payload = msg.payload<ceema::PayloadGroupMembers>();
            ThreeplGroup* group = m_groups.find_group(msg.sender(), payload.group);
            if (group) {
                group->set_members(payload.members);
                m_groups.update_chat(m_connection.acct(), *group);
            }
            break;
        }
        case ceema::MessageType::GROUP_LEAVE: {
            ceema::PayloadGroupLeave const& payload = msg.payload<ceema::PayloadGroupLeave>();
            ThreeplGroup* group = m_groups.find_group(m_connection.account().id(), payload.group);
            if (group && group->remove_member(msg.sender())) {
                ceema::PayloadGroupMembers payloadMembers;
                payloadMembers.group = group->gid();
                payloadMembers.members = group->members();

                sendPayload(m_connection.account(), group, payloadMembers);
            }
            break; }
        case ceema::MessageType::GROUP_TEXT: {
            ceema::PayloadGroupText const& payload = msg.payload<ceema::PayloadGroupText>();
            ThreeplGroup* group = find_or_create_group(payload.group, true);
            serv_got_chat_in(m_connection.connection(), group->id(), sender.c_str(), PURPLE_MESSAGE_RECV,
                             payload.m_text.c_str(), msg.time());
            break; }
        default:
            break;
    }
}

void ThreeplMessageHandler::send(ceema::Message& msg) {
    // Encrypt and send
    auto contactptr = m_contacts.get_contact(msg.recipient());
    if (!contactptr) {
        throw std::runtime_error("Unable to fetch contact key");
    }
    msg.encrypt(m_connection.account(), *contactptr);
    m_connection.send_packet(msg);
}
