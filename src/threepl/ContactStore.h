//
// Created by hbruintjes on 15/03/17.
//

#pragma once

#include <protocol/data/Client.h>
#include <contact/Contact.h>
#include <unordered_map>
#include <api/IdentAPI.h>
#include <memory>
#include <libpurple/account.h>

typedef std::shared_ptr<ceema::Contact> ContactPtr;

class ContactStore {
    std::unordered_map<ceema::client_id, ContactPtr> m_idmap;
    ceema::IdentAPI& m_ident_api;

public:
    ContactStore(ceema::IdentAPI& ident_api);

    bool has_contact(ceema::client_id const& id);

    ContactPtr get_contact(ceema::client_id const& id);

    void add_contact(ceema::Contact const& contact);

    ceema::future<ContactPtr> fetch_contact(ceema::client_id id);

    void update_buddies(PurpleAccount* account) const;
    void load_buddies(PurpleAccount* account);
};