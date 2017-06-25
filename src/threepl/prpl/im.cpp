//
// Created by harold on 30/05/17.
//

#include "im.h"

#include <threepl/ThreeplConnection.h>

int threepl_send_im(PurpleConnection* gc, const char *who, const char *message, PurpleMessageFlags) {
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    std::basic_string<char> text(message);
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

    auto msg_fut = connection->send_message(client, payload);
    if (!msg_fut) {
        return -ENOTCONN;
    } else {
        msg_fut.next([connection, msgtxt = std::basic_string<char>(message)](
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