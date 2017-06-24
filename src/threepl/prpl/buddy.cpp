//
// Created by hbruintjes on 14/03/17.
//

#include <string>
#include <contact/Contact.h>
#include <encoding/hex.h>
#include "buddy.h"
#include "threepl/ThreeplConnection.h"

void threepl_add_buddy_with_invite(PurpleConnection* gc, PurpleBuddy *buddy, PurpleGroup *group, const char *message) {
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    // Fetch the PK and store it if not already there
    const char *pk = purple_blist_node_get_string(&buddy->node, "public-key");
    if (!pk) {
        ceema::client_id id;
        try {
            id = ceema::client_id::fromString(buddy->name);
        } catch (std::exception& e) {
            purple_notify_error(gc, "Invalid Threema ID", "Unable to add buddy", e.what());
            purple_blist_node_set_string(&buddy->node, "public-key", "!");
            return;
        }
        auto fut_contact = connection->contact_store().fetch_contact(id);
        fut_contact.next([buddy, gc](ceema::future<ContactPtr> fut) {
            try {
                ContactPtr contact = fut.get();
                std::string pk_string = ceema::hex_encode(contact->pk());
                purple_blist_node_set_string(&buddy->node, "public-key", pk_string.c_str());
                purple_prpl_got_user_status(purple_buddy_get_account(buddy),
                                            purple_buddy_get_name(buddy),
                                            "online", NULL, NULL);
            } catch(std::exception& e) {
                purple_notify_error(gc, "Failed to get public-key", "Unable to retrieve public key for buddy", e.what());
                purple_blist_node_set_string(&buddy->node, "public-key", "!");
            }
            return true;
        });
    }
}
