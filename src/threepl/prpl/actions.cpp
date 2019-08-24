//
// Created by harold on 18-3-17.
//

#include "threepl/ThreeplConnection.h"
#include "threepl/ThreeplImport.h"
#include <libpurple/connection.h>
#include "actions.h"

//#include <libpurple/purple.h>
#include <contact/backup.h>
#include <libpurple/request.h>
#include <libpurple/debug.h>

#include <zip.h>

struct phone_cb_data {
    ThreeplConnection* connection;
    std::string verifyId;
};

static void threepl_set_nickname_cb(PurpleAccount* acct, const char* nickname) {
    purple_account_set_string(acct, "nickname", nickname);

    static PurpleAccount* callback_acct = acct;
    static const char* callback_nick = nickname;

    // Update existing chats to use new nickname
    purple_conversation_foreach([](PurpleConversation* conv) -> void {
        auto conv_acct = purple_conversation_get_account(conv);
        if (purple_conversation_get_type(conv) != PURPLE_CONV_TYPE_CHAT) {
            return;
        }
        if (conv_acct != callback_acct) {
            return;
        }

        purple_conv_chat_set_nick(purple_conversation_get_chat_data(conv), callback_nick);
    });
}

static void threepl_set_nickname(PurplePluginAction* action) {
    PurpleConnection* gc = static_cast<PurpleConnection*>(action->context);
    PurpleAccount* acct = purple_connection_get_account(gc);
    const char* oldnick = purple_account_get_string(acct, "nickname", "");

    purple_request_input(gc, ("Set User Nickname"), ("Please specify a new nickname for you."),
                         ("The nickname becomes visible when you send something to your contacts."
                                 " It can be at most 32 characters long."),
                         oldnick, FALSE, FALSE, NULL, ("Set"), PURPLE_CALLBACK(threepl_set_nickname_cb), ("Cancel"), NULL,
                         acct, NULL, NULL, acct);
}

static void threepl_set_email_cb(ThreeplConnection* connection, const char* email) {
    connection->ident_API().linkEmail(connection->account(), email).next([connection](ceema::future<bool> fut) {
        try {
            if (fut.get()) {
                purple_notify_info(connection->connection(), "E-mail linked", "Successfully linked e-mail address", NULL);
                return;
            }
        } catch(std::exception& e) {
            purple_notify_error(connection->connection(), "Failed to link email", "Unable to link email address", e.what());
            return;
        }
        purple_notify_error(connection->connection(), "Failed to link email", "Unable to link email address", NULL);
    });
}

static void threepl_set_email(PurplePluginAction* action) {
    PurpleConnection* gc = static_cast<PurpleConnection*>(action->context);
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    purple_request_input(gc, ("Set email address"), ("Specify the e-mail address to identify your account."),
                         ("This e-mail address can be used to find you as a contact."
                                 " You will receive a confirmation e-mail."),
                         NULL, FALSE, FALSE, NULL, ("Set"), PURPLE_CALLBACK(threepl_set_email_cb), ("Cancel"), NULL,
                         connection->acct(), NULL, NULL, connection);
}

static void threepl_set_phone_cb_verify(phone_cb_data* data, const char* code) {
    data->connection->ident_API().verifyMobile(data->connection->account(), data->verifyId, code).next([connection=data->connection](ceema::future<bool> fut) {
        try {
            fut.get();
            purple_notify_info(connection->connection(), "Phone linked", "Successfully linked phone number", NULL);
        } catch(std::exception& e) {
            purple_notify_error(connection->connection(), "Failed to link phone", "Unable to link phone number", e.what());
        }
    });

    delete data;
}

static void threepl_set_phone_code(phone_cb_data* data) {
    purple_request_input(data->connection->connection(), ("Set phone number"), ("Enter the confirmation code."),
                         ("You should receive a message or call with a confirmation code, which you need to enter here."),
                         NULL, FALSE, FALSE, NULL, ("Set"), PURPLE_CALLBACK(threepl_set_phone_cb_verify), ("Cancel"), NULL,
                         data->connection->acct(), NULL, NULL, data);
}

static void threepl_set_phone_action_verify(phone_cb_data* data, int action) {
    threepl_set_phone_code(data);
}

static void threepl_set_phone_action_call(phone_cb_data* data, int action) {
    data->connection->ident_API().callMobile(data->connection->account(), data->verifyId).next([data](ceema::future<bool> fut) {
        try {
            fut.get();
            threepl_set_phone_code(data);
        } catch(std::exception& e) {
            purple_notify_error(data->connection->connection(), "Failed to link phone", "Unable to link phone number", e.what());
            delete data;
        }
    });
}

static void threepl_set_phone_cb(ThreeplConnection* connection, const char* mobile) {
    bool unlink = (mobile[0] == 0); // No number means unlink
    connection->ident_API().linkMobile(connection->account(), mobile, false).next([connection, unlink](ceema::future<std::string> fut) {
        try {
            std::string verifyId = fut.get();
            if (verifyId.empty()) {
                    purple_notify_info(connection->connection(), unlink?("Phone unlinked"):("Phone linked"), "Successfully linked phone number", NULL);
                    return;
            }

            purple_request_action(connection->connection(), ("Verifying phone number"),
                                  ("To verify your phone number, wait for the verification code and press 'Verify'"),
                                  ("If you do not receive a message, press 'Call' to get an automated call"), 0, connection->acct(),
                                        NULL, NULL, new phone_cb_data({connection, verifyId}),
                                        2, "Verify", &threepl_set_phone_action_verify, "Call", &threepl_set_phone_action_call);
            return;
        } catch(std::exception& e) {
            purple_notify_error(connection->connection(), "Failed to link phone", "Unable to link phone number", e.what());
            return;
        }
        purple_notify_error(connection->connection(), "Failed to link phone", "Unable to link phone number", NULL);
    });
}

static void threepl_set_phone(PurplePluginAction* action) {
    PurpleConnection* gc = static_cast<PurpleConnection*>(action->context);
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    purple_request_input(gc, ("Set phone number"), ("Specify the phone number to identify your account."),
                         ("This phone number can be used to find you as a contact."
                                 " Please enter the full international number (E164 format) as digits only, without leading zeroes or plus"),
                         NULL, FALSE, FALSE, NULL, ("Submit"), PURPLE_CALLBACK(threepl_set_phone_cb), ("Cancel"), NULL,
                         connection->acct(), NULL, NULL, connection);
}

static void threepl_set_revocation_cb(ThreeplConnection* connection, const char* key) {
    connection->ident_API().setRevocationKey(connection->account(), key).next([connection](ceema::future<bool> fut) {
        try {
            purple_notify_info(connection->connection(), "Revocation key set", "Successfully set revocation key", "See https://myid.threema.ch/revoke");
            return;
        } catch(std::exception& e) {
            purple_notify_error(connection->connection(), "Failed to set revocation key", "Unable to set revocation key", e.what());
            return;
        }
        purple_notify_error(connection->connection(), "Failed to set revocation key", "Unable to set revocation key", NULL);
    });
}

static void threepl_set_revocation(PurplePluginAction* action) {
    PurpleConnection* gc = static_cast<PurpleConnection*>(action->context);
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    purple_request_input(gc, ("Set revocation key"), ("Specify the revocation key."),
                         ("The revocation key can be used to revoke (delete) your account."
                                 " See https://myid.threema.ch/revoke."),
                         NULL, FALSE, TRUE, NULL, ("Submit"), PURPLE_CALLBACK(threepl_set_revocation_cb), ("Cancel"), NULL,
                         connection->acct(), NULL, NULL, connection);
}

static void threepl_generate_backup_cb(ThreeplConnection* connection, const char* password) {
    try {
        std::string backup = ceema::make_backup(connection->account(), password);
        purple_notify_info(connection->connection(), "Backup string generated", "Backup string is displayed below", backup.c_str());
    } catch(std::exception& e) {
        purple_notify_error(connection->connection(), "Failed to generate backup string", "Unable to generate backup string", e.what());
    }
}

static void threepl_generate_backup(PurplePluginAction* action) {
    PurpleConnection* gc = static_cast<PurpleConnection*>(action->context);
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    purple_request_input(gc, ("Generate backup string"), ("Specify the password for the backup string."),
                         ("The backup string is encrypted using the password given here."),
                         NULL, FALSE, TRUE, NULL, ("Generate"), PURPLE_CALLBACK(threepl_generate_backup_cb), ("Cancel"), NULL,
                         connection->acct(), NULL, NULL, connection);
}

// Callback for import backup action - password/file is read - do the import
static void threepl_import_backup_doit(threepl_actions_data* data, char* password) {
    try {
        ThreeplImport importer (data->gc, password, data->text);
        ThreeplImport::imported num = importer.import_contacts ();

        if (num.b_new == -1) {
          purple_notify_error (data->connection->connection(),
                               "Failed to import from backup",
                               "Unable to read the backup file",
                              data->text);
        } else {
          std::string result = std::to_string (num.b_new)
                           + " new contacts imported, "
                           + std::to_string (num.b_old)
                           + " already exisiting";
          purple_notify_info (data->connection->connection(), 
                              "Contacts and groups imported",
                              "Successfully read data from backup file",
                              result.c_str ());
        }
    } 
    catch(std::exception& e) {
        purple_notify_error(data->connection->connection(), "Failed to generate backup string", "Unable to import data from backup", e.what());
    }

    delete data;
}

// Callback for import backup action - password is read - get name of archive file next
static void threepl_import_password (threepl_actions_data* data, const char* backup_file) {

    strcpy (data->text, backup_file);

    // ask user for the password of the zip-archive
    purple_request_input (data->gc, ("Backup Password"), ("Enter the password of the backup archive."),
                          ("The password is required for decrypting the archive."),
                          NULL, FALSE, TRUE, NULL, ("OK"), PURPLE_CALLBACK(threepl_import_backup_doit), ("Cancel"), NULL,
                          data->connection->acct(), NULL, NULL, data);
}

// Callback for import backup action
static void threepl_import_backup(PurplePluginAction* action) {

    PurpleConnection* gc = static_cast<PurpleConnection*>(action->context);
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    // initialize user data
    threepl_actions_data* data = new threepl_actions_data ();
    data->gc = gc;
    data->connection = connection;

    // get backup file
    purple_request_file (gc, ("Backup file (zip-archive) to import from"),
                         ("Contacts and groups will be imported into the buddy list."),
                         FALSE, PURPLE_CALLBACK(threepl_import_password), NULL,
                         connection->acct(), NULL, NULL, data);
}

GList* threepl_actions(PurplePlugin *plugin, gpointer context) {
    PurplePluginAction *act = purple_plugin_action_new(("Set Nickname..."), &threepl_set_nickname);
    GList* acts = g_list_append(NULL, act);
    act = purple_plugin_action_new(("Link e-mail address..."), &threepl_set_email);
    acts = g_list_append(acts, act);
    act = purple_plugin_action_new(("Link phone number..."), &threepl_set_phone);
    acts = g_list_append(acts, act);
    act = purple_plugin_action_new(("Set revocation key..."), &threepl_set_revocation);
    acts = g_list_append(acts, act);
    act = purple_plugin_action_new(("Import from backup..."), &threepl_import_backup);
    acts = g_list_append(acts, act);
    act = purple_plugin_action_new(("Generate backup string..."), &threepl_generate_backup);
    return g_list_append(acts, act);

}
