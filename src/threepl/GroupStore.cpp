//
// Created by hbruintjes on 17/03/17.
//

#include <libpurple/debug.h>
#include <cstring>
#include <logging/logging.h>
#include "GroupStore.h"

PurpleConvChat* ThreeplGroup::find_conversation(PurpleConnection *gc) const {
    auto conv = purple_find_chat(gc, id());
    return conv ? PURPLE_CONV_CHAT(conv) : NULL;
}

PurpleConvChat* ThreeplGroup::create_conversation(PurpleConnection *gc) const {
    auto conv = find_conversation(gc);
    if (conv) {
        return conv;
    }

    conv = PURPLE_CONV_CHAT(serv_got_joined_chat(gc, id(), chat_name().c_str()));
    // Set current nickname
    const char* nick = purple_account_get_string(purple_connection_get_account(gc),
                                                 "nickname", "");
    purple_conv_chat_set_nick(conv, nick);

    // Set initial members
    GList* users = NULL;
    GList* flags = NULL;
    for(ceema::client_id const& cid: members()) {
        users = g_list_append(users, g_strdup(cid.toString().c_str()));
        flags = g_list_append(flags, GINT_TO_POINTER( cid == owner() ? PURPLE_CBFLAGS_FOUNDER : PURPLE_CBFLAGS_NONE ));
    }
    purple_conv_chat_add_users(conv, users, NULL, flags, false);

    // Update group data
    update_conversation(conv);

    return conv;
}

void ThreeplGroup::update_conversation(PurpleConvChat *conv) const {
    // Set topic
    const char* topic = purple_conv_chat_get_topic(conv);
    if (!topic || name() != topic) {
        purple_conv_chat_set_topic(conv, owner().toString().c_str(),
                                   name().c_str());
    }

    // Set members
    GList* new_users = NULL;
    GList* users = NULL;
    GList* new_flags = NULL;
    GList* flags = NULL;
    for(ceema::client_id const& cid: members()) {
        if (purple_conv_chat_find_user(conv, cid.toString().c_str())) {
            users = g_list_append(users, g_strdup(cid.toString().c_str()));
            flags = g_list_append(flags, GINT_TO_POINTER( cid == owner() ? PURPLE_CBFLAGS_FOUNDER : PURPLE_CBFLAGS_NONE ));
        } else {
            new_users = g_list_append(new_users, g_strdup(cid.toString().c_str()));
            new_flags = g_list_append(new_flags, GINT_TO_POINTER( cid == owner() ? PURPLE_CBFLAGS_FOUNDER : PURPLE_CBFLAGS_NONE ));
        }

    }
    purple_conv_chat_clear_users(conv);
    purple_conv_chat_add_users(conv, users, NULL, flags, false);
    purple_conv_chat_add_users(conv, new_users, NULL, new_flags, true);

    g_list_free_full(users, &g_free);
    g_list_free(flags);
    g_list_free_full(new_users, &g_free);
    g_list_free(new_flags);
}

PurpleChat* ThreeplGroup::find_blist_chat(PurpleAccount *account) const {
    return purple_blist_find_chat(account, chat_name().c_str());
}

void ThreeplGroup::update_blist_chat(PurpleChat *chat) const {
    purple_blist_node_set_string(&chat->node, "name", name().c_str());
    if (!name().empty()) {
        // Alias the chat to the title, if not set to anything else
        // TODO: tramples possible user-set alias
        const char* orig_alias = chat->alias;
        if (!orig_alias || (name() != orig_alias)) {
            purple_blist_alias_chat(chat, name().c_str());
        }
    }

    std::string memberlist;
    memberlist.reserve(members().size() * ceema::client_id::array_size + 1);
    for(auto const& cid: members()) {
        memberlist.append(cid.toString());
    }
    purple_blist_node_set_string(&chat->node, "members", memberlist.c_str());
}

void ThreeplGroup::set_name(std::string name) {
    if (name.size()) {
        m_name = std::move(name);
    } else {
        m_name = ceema::hex_encode(m_groupid) + " owned by " + m_owner.toString();
    }
}

ThreeplGroup* GroupStore::find_or_create(GHashTable* components) {
    const char* owner = static_cast<const char*>(g_hash_table_lookup(components, "owner"));
    ceema::client_id owner_id = ceema::client_id::fromString(owner);

    const char* id = static_cast<const char*>(g_hash_table_lookup(components, "id"));
    ceema::group_id gid;
    ceema::hex_decode(std::string(id), gid);

    return find_group(owner_id, gid, true);
}

PurpleChat* GroupStore::find_chat(PurpleAccount* account, ThreeplGroup const& group) const {
    return group.find_blist_chat(account);
}

void GroupStore::update_chat(PurpleAccount* account, ThreeplGroup const& group) const {
    // Update stored data if any
    PurpleChat* chat = group.find_blist_chat(account);
    if (chat) {
        group.update_blist_chat(chat);
    }

    // Update conversation if any
    PurpleConvChat* chat_conv = group.find_conversation(purple_account_get_connection(account));
    if (chat_conv) {
        group.update_conversation(chat_conv);
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
        const char* name = purple_blist_node_get_string(&chat->node, "name");

        ceema::group_id gid;
        ceema::hex_decode(std::string(id), gid);
        ceema::client_id owner_id = ceema::client_id::fromString(owner);

        ThreeplGroup* group = add_group(owner_id, gid);
        if (name && name[0] != 0) {
            group->set_name(name);
        }
        if (members && (strlen(members) % ceema::client_id::array_size) == 0) {
            size_t len = strlen(members);
            for(size_t i = 0; i < len; i += ceema::client_id::array_size) {
                std::string idStr = std::string(members+i, ceema::client_id::array_size);
                group->add_member(ceema::client_id::fromString(idStr));
            }
        }
    }
}