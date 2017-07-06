//
// Created by harold on 20/05/17.
//

#include <glib.h>
#include <libpurple/cmds.h>
#include <vector>
#include "threepl/ThreeplConnection.h"

class Command {
    PurpleCmdId m_id;
public:
    explicit Command(PurpleCmdId id) : m_id(id) {}
    ~Command() {
        if (m_id != 0) {
            purple_cmd_unregister(m_id);
        }
    }

    Command(Command const&) = delete;
    Command(Command && other) : m_id(other.m_id) {
        other.m_id = 0;
    }

    Command& operator=(Command const&) = delete;
    Command& operator=(Command && other) {
        std::swap(m_id, other.m_id);
        return *this;
    }
};

static std::vector<Command> commands;

PurpleCmdRet threepl_message_agree(PurpleConversation * conv, bool agree) {
    PurpleAccount* acct = purple_conversation_get_account(conv);
    PurpleConnection* gc = purple_account_get_connection(acct);
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    const char* who = purple_conversation_get_name(conv);

    try {
        if (connection->send_agreement(ceema::client_id::fromString(who),
                                       agree)) {
            return PURPLE_CMD_RET_OK;
        }
    } catch (std::runtime_error& e) {
        // Invalid ID
        return PURPLE_CMD_RET_FAILED;
    }

    return PURPLE_CMD_RET_FAILED;
}

PurpleCmdRet threepl_command_agree(PurpleConversation * conv, const gchar *,
                                   gchar **, gchar **, void *) {
    return threepl_message_agree(conv, true);
}

PurpleCmdRet threepl_command_disagree(PurpleConversation * conv, const gchar *,
                                   gchar **, gchar **, void *) {
    return threepl_message_agree(conv, false);
}

PurpleCmdRet threepl_command_sync(PurpleConversation * conv, const gchar *,
                                   gchar **, gchar **, void *) {
    PurpleAccount* acct = purple_conversation_get_account(conv);
    PurpleConnection* gc = purple_account_get_connection(acct);
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    PurpleConvChat* conv_chat = PURPLE_CONV_CHAT(conv);
    int id = purple_conv_chat_get_id(conv_chat);
    ThreeplGroup* group_data = connection->group_store().find_group(id);
    if (group_data && group_data->owner() != connection->account().id()) {
        ceema::PayloadGroupSync payload;
        payload.group = group_data->gid();
        connection->send_message(group_data->owner(), payload);
    } else{
        return PURPLE_CMD_RET_FAILED;
    }

    return PURPLE_CMD_RET_OK;
}

void threepl_register_commands(PurplePlugin*) {
    if (!commands.empty()) {
        return;
    }

    PurpleCmdId id = purple_cmd_register("disagree", "", PURPLE_CMD_P_PRPL,
                                     static_cast<PurpleCmdFlag>(PURPLE_CMD_FLAG_IM ),
                                     "prpl-threepl", threepl_command_disagree,
                                     "disagree:  Disagree with last message received.", nullptr);
    commands.emplace_back(id);
    id = purple_cmd_register("agree", "", PURPLE_CMD_P_PRPL,
                             static_cast<PurpleCmdFlag>(PURPLE_CMD_FLAG_IM ),
                             "prpl-threepl", threepl_command_agree,
                             "agree:  Agree with last message received.", nullptr);
    commands.emplace_back(id);

    id = purple_cmd_register("sync", "", PURPLE_CMD_P_PRPL,
                             static_cast<PurpleCmdFlag>(PURPLE_CMD_FLAG_CHAT ),
                             "prpl-threepl", threepl_command_sync,
                             "sync:  Request group sync.", nullptr);
    commands.emplace_back(id);
}

void threepl_unregister_commands(PurplePlugin*) {
    commands.clear();
}

