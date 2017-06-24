//
// Created by harold on 30/05/17.
//

#include "connection.h"

#include <threepl/ThreeplConnection.h>
#include <contact/backup.h>

void threepl_login(PurpleAccount *account) {
    PurpleConnection* gc = purple_account_get_connection(account);

    const char* username = purple_account_get_username(account);
    const char* password = purple_account_get_password(account);
    const char* backup   = purple_account_get_string(account, "backup", "");

    try {
        ceema::Account c = *backup ? ceema::decrypt_backup(backup, password) :
                           ceema::Account(ceema::client_id::fromString(username), ceema::hex_decode<ceema::private_key>(
                                   std::string(password)));

        if (c.id().toString() != username) {
            gchar* msg = g_strdup_printf("The user ID '%s' does not match the backup string ID '%s'", username, c.id().toString().c_str());
            purple_notify_warning(gc, "Invalid username", "Username mismatch", msg);
            g_free(msg);
        }

        ThreeplConnection* connection = new ThreeplConnection(account, c);

        // Initialize known contacts
        connection->contact_store().load_buddies(account);

        // Load chats
        connection->group_store().load_chats(account);

        purple_connection_set_protocol_data(gc, connection);
        connection->start_connect();
    } catch (std::exception& e) {
        purple_notify_error(gc, "Login error", "Invalid username or password",
                            "Unable to decrypt the backup string, please check account settings.");
        return;
    }
}

void threepl_close(PurpleConnection *gc) {
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    if (!connection) {
        return;
    }

    connection->close();

    connection->contact_store().update_buddies(connection->acct());
    connection->group_store().update_chats(connection->acct());

    delete connection;
    purple_connection_set_protocol_data(gc, nullptr);
}

void threepl_keepalive(PurpleConnection* gc) {
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    connection->send_keepalive();
}