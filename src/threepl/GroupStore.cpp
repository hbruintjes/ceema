//
// Created by hbruintjes on 17/03/17.
//

#include <libpurple/debug.h>
#include <cstring>
#include "GroupStore.h"

ThreeplGroup* GroupStore::find_or_create(GHashTable* components) {
    const char* owner = static_cast<const char*>(g_hash_table_lookup(components, "owner"));
    ceema::client_id owner_id = ceema::client_id::fromString(owner);

    const char* id = static_cast<const char*>(g_hash_table_lookup(components, "id"));
    ceema::group_id gid;
    ceema::hex_decode(std::string(id), gid);

    return find_group(owner_id, gid, true);
}

PurpleChat* GroupStore::find_chat(PurpleAccount* account, ThreeplGroup const& group) const {
    return purple_blist_find_chat(account, group.chat_name().c_str());
}

void GroupStore::update_chat(PurpleAccount* account, ThreeplGroup const& group) const {
    // Update stored data if any
    PurpleChat* chat = purple_blist_find_chat(account, group.chat_name().c_str());
    if (chat) {
        purple_blist_node_set_string(&chat->node, "name", group.name().c_str());
        char* memberlist = new char[group.members().size() * ceema::client_id::array_size + 1];
        char* iter = memberlist;
        for(auto const& cid: group.members()) {
            iter = std::copy(cid.begin(), cid.end(), iter);
        }
        *iter = '\0';
        purple_blist_node_set_string(&chat->node, "members", memberlist);
        delete [] memberlist;
    }
    // Update conversation if any

    PurpleConversation* conv = purple_find_chat(purple_account_get_connection(account), group.id());
    PurpleConvChat* chat_conv = NULL;
    if (conv) {
        chat_conv = purple_conversation_get_chat_data(conv);
    }
    if (chat_conv) {
        purple_conv_chat_clear_users(chat_conv);
        GList* users = NULL;
        GList* flags = NULL;
        for(ceema::client_id const& cid: group.members()) {
            users = g_list_append(users, g_strdup(cid.toString().c_str()));
            flags = g_list_append(flags, GINT_TO_POINTER( cid == group.owner() ? PURPLE_CBFLAGS_FOUNDER : PURPLE_CBFLAGS_NONE ));
        }
        purple_conv_chat_add_users(chat_conv, users, NULL, flags, false);

        g_list_free_full(users, &g_free);
        g_list_free(flags);

        // Set title
        purple_conversation_set_title(conv, group.name().c_str());

        // Set current nickname
        const char* nick = purple_account_get_string(account, "nickname", "");
        purple_conv_chat_set_nick(chat_conv, nick);
    }
}

void GroupStore::update_chats(PurpleAccount* account) const {
    for(auto& group: m_groups) {
        update_chat(account, group);
    }
}

void GroupStore::load_chats(PurpleAccount* account) {
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
        const char* id = static_cast<const char*>(g_hash_table_lookup(components, "id"));
        const char* owner = static_cast<const char*>(g_hash_table_lookup(components, "owner"));
        const char* members = purple_blist_node_get_string(&chat->node, "members");

        ceema::group_id gid;
        ceema::hex_decode(std::string(id), gid);
        ceema::client_id owner_id = ceema::client_id::fromString(owner);

        ThreeplGroup* group = add_group(owner_id, gid);
        if (members && (strlen(members) % ceema::client_id::array_size) == 0) {
            size_t len = strlen(members);
            for(size_t i = 0; i < len; i += ceema::client_id::array_size) {
                std::string idStr = std::string(members+i, ceema::client_id::array_size);
                group->add_member(ceema::client_id::fromString(idStr));
            }
        }
    }
}