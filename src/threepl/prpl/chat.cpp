//
// Created by harold on 30/05/17.
//

#include "chat.h"

#include <threepl/ThreeplConnection.h>

char* threepl_get_chat_name(GHashTable *components) {
    const char* id = static_cast<const char*>(g_hash_table_lookup(components, "id"));
    const char* owner = static_cast<const char*>(g_hash_table_lookup(components, "owner"));

    // The following would be better, but libpurple does not provide account or connection
    //ThreeplGroup* group_data = connection->group_store().find_or_create(components);

    try {
        std::string chat_name = ThreeplGroup::chat_name(
                ceema::group_id(ceema::hex_decode(std::string(id))),
                ceema::client_id::fromString(owner));
        return g_strdup(chat_name.c_str());
    } catch (std::exception& e) {
        // Crappy input, return empty string
        return g_strdup("");
    }
}

void threepl_join_chat(PurpleConnection *gc, GHashTable* components) {
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    ThreeplGroup* group_data = connection->group_store().find_or_create(components);
    group_data->create_conversation(gc);
}

/*
void threepl_reject_chat(PurpleConnection *gc, GHashTable *components) {
    // Threema does not support rejecting groups
}

void threepl_chat_leave(PurpleConnection *gc, int id) {
    PurpleConversation *conv = purple_find_chat(gc, id);
    //TODO: send leave message, or destroy the group?
}
*/

GList* threepl_chat_info(PurpleConnection*) {
    struct proto_chat_entry* pce = g_new0(struct proto_chat_entry, 1);
    pce->label = ("ID");
    pce->identifier = "id";
    pce->required = TRUE;
    GList* info_list = g_list_append(NULL, pce);
    pce = g_new0(struct proto_chat_entry, 1);
    pce->label = ("Owner");
    pce->identifier = "owner";
    pce->required = TRUE;
    info_list = g_list_append(info_list, pce);

    return info_list;
}

GHashTable* threepl_chat_info_defaults(PurpleConnection* gc, const char* room) {
    // Room is the name as formed by threepl_get_chat_name
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    GHashTable* defaults = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);

    bool has_default = false;
    if (room) {
        gchar* groomid = g_strdup(room);
        gchar* groomowner = strchr(groomid, '@');
        if (groomowner) {
            *groomowner ='\0';
            groomowner++;
            // Generate new room info
            g_hash_table_insert(defaults, g_strdup("id"), g_strdup(groomid));
            g_hash_table_insert(defaults, g_strdup("owner"), g_strdup(groomowner));
            has_default = true;
        }
        g_free(groomid);
    }
    if (!has_default) {
        // Generate new room info
        g_hash_table_insert(defaults, g_strdup("id"), g_strdup(
                ceema::hex_encode(ceema::gen_group_id()).c_str()));
        g_hash_table_insert(defaults, g_strdup("owner"), g_strdup(connection->account().id().toString().c_str()));
    }
    return defaults;
}

PurpleChat* threepl_find_blist_chat(PurpleAccount* account, const char* name) {
    for(PurpleBlistNode* bnode = purple_blist_get_root(); bnode;
        bnode = purple_blist_node_next(bnode, TRUE)) {
        if(!PURPLE_BLIST_NODE_IS_CHAT(bnode)) {
            continue;
        }

        PurpleChat* chat = PURPLE_CHAT(bnode);
        if (purple_chat_get_account(chat) != account) {
            continue;
        }

        GHashTable* components = purple_chat_get_components(chat);
        char* chatname = threepl_get_chat_name(components);
        if (!strcmp(name, chatname)) {
            g_free(chatname);
            return chat;
        }
        g_free(chatname);
    }
    return NULL;
}

void threepl_chat_invite(PurpleConnection* gc, int id, const char* message, const char* who) {
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    PurpleConversation* conv = purple_find_chat(gc, id);
    ThreeplGroup* group_data = connection->group_store().find_group(id);
    if (!conv || !group_data) {
        purple_notify_error(gc, "Error", "Unable to invite user", "The group does not exist");
        return;
    }

    ceema::client_id client;
    try {
        client = ceema::client_id::fromString(who);
    } catch(std::exception& e) {
        purple_notify_error(gc, "Error", "Unable to invite user", e.what());
        return;
    }
    if (group_data->add_member(client)) {
        // Added new member, put in group
        ceema::PayloadGroupMembers payloadMembers;
        payloadMembers.group = group_data->gid();
        payloadMembers.members = group_data->members();
        connection->send_group_message(group_data, payloadMembers);

        ceema::PayloadGroupTitle payloadTitle;
        payloadTitle.group = group_data->gid();
        payloadTitle.title = group_data->name();
        connection->send_group_message(group_data, payloadTitle);

        connection->group_store().update_chat(connection->acct(), *group_data);
    }
}

int threepl_chat_send(PurpleConnection* gc, int id, const char* message, PurpleMessageFlags flags) {
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    PurpleConversation* conv = purple_find_chat(gc, id);
    ThreeplGroup* group_data = connection->group_store().find_group(id);
    if (!conv || !group_data) {
        purple_notify_error(gc, "Error", "Unable to send message", "The group does not exist");
        return -ENOENT;
    }

    ceema::PayloadGroupText payload;
    payload.group = group_data->uid();
    auto raw_message = purple_unescape_html(message);
    payload.m_text = raw_message;
    g_free(raw_message);
    auto msg_fut_vec = connection->send_group_message(group_data, payload);
    if (msg_fut_vec.empty()) {
        return -ENOTCONN;
    }
    bool first = true;
    for(auto& msg_fut: msg_fut_vec) {
        if (first) {
            first = false;
            msg_fut.next([connection, id, msg = std::string(message), flags](
                    ceema::future<std::unique_ptr<ceema::Message>> fut) {
                try {
                    std::unique_ptr<ceema::Message> m = fut.get();
                    serv_got_chat_in(connection->connection(), id, m->sender().toString().c_str(), flags, msg.c_str(), m->time());
                } catch (message_exception &e) {
                    const char* who = e.id().toString().c_str();
                    gchar *errMsg = g_strdup_printf("Unable to send message to %s: %s", who, e.what());
                    if (!purple_conv_present_error(who, connection->acct(), errMsg)) {
                        purple_notify_error(connection->connection(), "Error sending message", "Unable to send message", errMsg);
                    }
                    g_free(errMsg);
                }

            });
        } else {
            msg_fut.next([connection](ceema::future<std::unique_ptr<ceema::Message>> fut) {
                try {
                    std::unique_ptr<ceema::Message> m = fut.get();
                } catch (message_exception &e) {
                    std::string who = e.id().toString();
                    gchar *errMsg = g_strdup_printf("Unable to send message to %s: %s", who.c_str(), e.what());
                    if (!purple_conv_present_error(who.c_str(), connection->acct(), errMsg)) {
                        purple_notify_error(connection->connection(), "Error sending message", "Unable to send message",
                                            errMsg);
                    }
                    g_free(errMsg);
                }

            });
        }
    }

    // Echo the message regardless of error (some clients may succeed while others fail)
    return 1;
}

void threepl_set_chat_topic(PurpleConnection *gc, int id, const char *topic) {
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    ThreeplGroup* group_data = connection->group_store().find_group(id);
    if (!group_data) {
        purple_notify_error(gc, "Error", "Unable to set topic", "The group does not exist");
        return;
    }

    if (group_data->owner() == connection->account().id()) {
        group_data->set_name(topic);
        connection->group_store().update_chat(connection->acct(), *group_data);
        ceema::PayloadGroupTitle payload;
        payload.group = group_data->gid();
        payload.title = topic;
        connection->send_group_message(group_data, payload);
    }

}
