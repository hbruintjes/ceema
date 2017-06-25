//
// Created by hbruintjes on 15/03/17.
//

#include "ContactStore.h"

#include <libpurple/debug.h>
#include <libpurple/blist.h>
#include <encoding/hex.h>

ContactStore::ContactStore(ceema::IdentAPI& ident_api) : m_ident_api(ident_api) {}

bool ContactStore::has_contact(ceema::client_id const& id) {
    auto search = m_idmap.find(id);
    return (search != m_idmap.end());
}

ContactPtr ContactStore::get_contact(ceema::client_id const& id) {
    return m_idmap.find(id)->second;
    //return m_idmap[id];
}

void ContactStore::add_contact(ceema::Contact const& contact) {
    m_idmap.emplace(contact.id(), std::make_shared<ceema::Contact>(contact));
}

ceema::future<ContactPtr> ContactStore::fetch_contact(ceema::client_id id) {
    if (has_contact(id)) {
        ceema::promise<ContactPtr> prom;
        prom.set_value(m_idmap[id]);
        return prom.get_future();
    }
    auto fut_contact = m_ident_api.getClientInfo(id.toString());
    return fut_contact.next([this, id](ceema::future<ceema::Contact> fut) {
        try {
            auto contact_ptr = std::make_shared<ceema::Contact>(fut.get());
            m_idmap.emplace(id, contact_ptr);
            return contact_ptr;
        } catch (std::exception& e) {
            LOG_DBG("Failed to fetch contact: " << e.what());
            m_idmap.emplace(id, nullptr);
            throw; //std::rethrow_exception(std::current_exception());
        }
    });
}

void ContactStore::update_buddies(PurpleAccount* account) const {
    for(auto& contact_pair: m_idmap) {

        auto idStr = contact_pair.first.toString();
        PurpleBuddy* buddy = purple_find_buddy(account, idStr.c_str());
        if (buddy) {
            if (!contact_pair.second) {
                purple_blist_node_set_string(&buddy->node, "public-key",
                                             "!");
            } else {
                purple_blist_node_set_string(&buddy->node, "public-key",
                                             ceema::hex_encode(
                                                     contact_pair.second->pk()).c_str());
            }
        }
    }
}

void ContactStore::load_buddies(PurpleAccount* account) {
    PurpleConnection* gc = purple_account_get_connection(account);

    for (GSList* buddies = purple_find_buddies(account, NULL); buddies;
         buddies = g_slist_delete_link(buddies, buddies)) {
        PurpleBuddy* buddy = static_cast<PurpleBuddy*>(buddies->data);
        const char* client_id_str = purple_buddy_get_name(buddy);

        // Retrieve last-known alias
        const char* nickname = purple_blist_node_get_string(&buddy->node, "nickname");
        if (nickname) {
            serv_got_alias(gc, client_id_str, nickname);
        }

        // Convert PK to Contact
        ceema::client_id id;
        try {
            id = ceema::client_id::fromString(client_id_str);
        } catch (std::runtime_error& e) {
            // Invalid ID, broken buddy
            continue;
        }
        if (!has_contact(id)) {
            const char *pk = purple_blist_node_get_string(&buddy->node, "public-key");
            if (pk) {
                if (*pk == '!') {
                    m_idmap.emplace(id, nullptr);
                } else {
                    std::string pk_string{pk};
                    add_contact(ceema::Contact(id, ceema::public_key{
                            ceema::hex_decode(pk_string)}));
                }
            }
        }

        if (has_contact(id)) {
            purple_prpl_got_user_status(account, purple_buddy_get_name(buddy), "online", NULL, NULL);
        }
    }
}
