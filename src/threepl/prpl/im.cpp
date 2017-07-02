//
// Created by harold on 30/05/17.
//

#include "im.h"

#include <threepl/ThreeplConnection.h>

/**
 * Sends current avatar to who if not send since last update,
 * and who is in the buddy list
 * @param who ID of user to send avatar to
 */
void send_avatar(ThreeplConnection* connection, ceema::client_id who) {
    PurpleAccount* account = connection->acct();

    PurpleBuddy* buddy = purple_find_buddy(account, who.toString().c_str());
    if (!buddy) {
        return;
    }

    // Timestamp stored as string so no overflows etc.
    const char* str_timestamp = purple_blist_node_get_string(&buddy->node, "icon-ts");
    time_t timestamp = str_timestamp ? std::strtoul(str_timestamp, nullptr, 10) : 0;

    auto img = purple_buddy_icons_find_account_icon(account);
    if (!img) {
        if (timestamp != 0) {
            // Clear the icon
            ceema::PayloadIconClear payload;
            connection->send_message(who, payload);
            purple_blist_node_set_string(&buddy->node, "icon-ts", "0");
        }
        return;
    }
    std::uint8_t const *avatar_data = static_cast<std::uint8_t const*>(purple_imgstore_get_data(img));
    auto avatar_len = purple_imgstore_get_size(img);
    time_t icon_ts = purple_buddy_icons_get_account_icon_timestamp(account);



    if (timestamp < icon_ts) {
        // Send updated icon
        auto key = ceema::crypto::generate_shared_key();
        ceema::byte_vector data{avatar_data, avatar_data+avatar_len};
        ceema::BlobUploadTransfer *transfer = new ceema::BlobUploadTransfer(data, ceema::BlobType::ICON, key);
        transfer->get_future().next([connection, who](ceema::future<ceema::Blob> fut) {
            ceema::Blob blob;
            try {
                blob = fut.get();
            } catch (std::exception& e) {
                LOG_DBG("Failed to upload avatar: " << e.what());
                throw;
            }

            ceema::PayloadIcon payload{blob.id, blob.size, blob.key};
            return connection->send_message(who, payload);
        }).next([](ceema::future<std::unique_ptr<ceema::Message>> fut) {
            try {
                fut.get();
            } catch (std::exception& e) {
                LOG_DBG("Failed to send avatar: " << e.what());
            }
        });
        connection->blob_API().upload(transfer);
        purple_blist_node_set_string(&buddy->node, "icon-ts", std::to_string(icon_ts).c_str());
    }

    purple_imgstore_unref(img);
}

int threepl_send_im(PurpleConnection* gc, const char *who, const char *message, PurpleMessageFlags) {
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    auto raw_message = purple_unescape_html(message);
    std::string text(raw_message);
    g_free(raw_message);
    ceema::PayloadText payload;
    payload.m_text = text;

    ceema::client_id client;
    try {
        client = ceema::client_id::fromString(who);
    } catch(std::exception& e) {
        gchar *errMsg = g_strdup_printf("Unable to send message to %s: %s", who, e.what());
        if (!purple_conv_present_error(who, connection->acct(), errMsg)) {
            purple_notify_error(connection->connection(), "Error sending message", "Unable to send message", errMsg);
        }
        g_free(errMsg);
        return 0;
    }

    send_avatar(connection, client);

    auto msg_fut = connection->send_message(client, payload);
    if (!msg_fut) {
        return -ENOTCONN;
    } else {
        msg_fut.next([connection, msgtxt = std::string(message)](
                ceema::future<std::unique_ptr<ceema::Message>> fut){
            try {
                auto msg = fut.get();
                std::string who_rcpt = msg->recipient().toString();
                PurpleConversation* conv = purple_find_conversation_with_account(
                        PURPLE_CONV_TYPE_IM, who_rcpt.c_str(), connection->acct());
                if (conv) {
                    purple_conv_im_write(PURPLE_CONV_IM(conv), who_rcpt.c_str(), msgtxt.c_str(), PURPLE_MESSAGE_SEND, msg->time());
                }
            } catch (message_exception& e) {
                std::string who_err = e.id().toString();
                gchar *errMsg = g_strdup_printf("Unable to send message to %s: %s", who_err.c_str(), e.what());
                if (!purple_conv_present_error(who_err.c_str(), connection->acct(), errMsg)) {
                    purple_notify_error(connection->connection(), "Error sending message", "Unable to send message", errMsg);
                }
                g_free(errMsg);
            }

        });
        return 0;
    }
}

unsigned int threepl_send_typing(PurpleConnection* gc, const char* who, PurpleTypingState state) {
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    ceema::PayloadTyping payload;
    payload.m_typing = (state == PURPLE_TYPING);

    try {
        connection->send_message(ceema::client_id::fromString(who), payload);
    } catch (std::exception& e) {
        // Log error, not much more to do
        LOG_ERROR(ceema::logging::loggerRoot, e.what());
    }

    return 0;
}