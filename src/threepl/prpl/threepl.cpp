/**
 * purple
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * Nullprpl is a mock protocol plugin for Pidgin and libpurple. You can create
 * accounts with it, sign on and off, add buddies, and send and receive IMs,
 * all without connecting to a server!
 *
 * Beyond that basic functionality, nullprpl supports presence and
 * away/available messages, offline messages, user info, typing notification,
 * privacy allow/block lists, chat rooms, whispering, room lists, and protocol
 * icons and emblems. Notable missing features are file transfer and account
 * registration and authentication.
 *
 * Nullprpl is intended as an example of how to write a libpurple protocol
 * plugin. It doesn't contain networking code or an event loop, but it does
 * demonstrate how to use the libpurple API to do pretty much everything a prpl
 * might need to do.
 *
 * Nullprpl is also a useful tool for hacking on Pidgin, Finch, and other
 * libpurple clients. It's a full-featured protocol plugin, but doesn't depend
 * on an external server, so it's a quick and easy way to exercise test new
 * code. It also allows you to work while you're disconnected.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

// Link as plugin library
#define PURPLE_PLUGINS 1

#include <string.h>

#include <glib.h>

#include <protocol/session.h>
#include "buddy.h"

#include "libpurple/version.h"
#include "libpurple/accountopt.h"

#include "threepl.h"
#include "threepl/ThreeplConnection.h"
#include "actions.h"
#include "commands.h"
#include "threepl/Logging.h"

#include "chat.h"
#include "list.h"
#include "xfer.h"
#include "im.h"
#include "connection.h"

static GList* threepl_status_types(PurpleAccount *) {
    GList *types = NULL;
    PurpleStatusType *type;

    type = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE, "online", NULL, TRUE, TRUE, FALSE);
    types = g_list_prepend(types, type);


    type = purple_status_type_new_full(PURPLE_STATUS_OFFLINE, "offline", NULL, TRUE, TRUE, FALSE);
    types = g_list_prepend(types, type);

    //TODO: away reserved for inactive accounts (3 months unused)

    return g_list_reverse(types);
}

static GHashTable* threepl_get_account_text_table(PurpleAccount*) {
    GHashTable* table = g_hash_table_new(g_str_hash, g_str_equal);

    g_hash_table_insert(table, g_strdup("login_label"), g_strdup("Threema ID"));

    return table;
}

static void threepl_get_info(PurpleConnection* gc, const char* who) {
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));

    PurpleNotifyUserInfo* user_info = purple_notify_user_info_new();
    purple_notify_user_info_add_pair_plaintext(user_info, "ID", who);

    ceema::client_id cid = ceema::client_id::fromString(who);
    if (connection->contact_store().has_contact(cid)) {
        ContactPtr contact = connection->contact_store().get_contact(cid);
        purple_notify_user_info_add_pair_plaintext(user_info, "Public key", ceema::hex_encode(contact->pk()).c_str());
        purple_notify_user_info_add_pair_plaintext(user_info, "Nickname", contact->nick().c_str());
        //Fingerprint
        //QR code
        //Icon
    }

    purple_notify_userinfo(gc, who, user_info, [](gpointer){}, NULL);
}

static gboolean threepl_offline_message(const PurpleBuddy *) {
    // Despite being FALSE, pidgin happily tries to send messages
    return FALSE;
}

/*
 * prpl stuff. see prpl.h for more information.
 */

static char icon_types[] = "jpg,png";

static PurplePluginProtocolInfo threepl_protocol_info =
{
  static_cast<PurpleProtocolOptions>(OPT_PROTO_CHAT_TOPIC),  /* options */
  NULL,               /* user_splits, initialized in threepl_init() */
  NULL,               /* protocol_options, initialized in threepl_init() */
  {icon_types, 1, 1, 512, 512, 20*1024*1024, PURPLE_ICON_SCALE_SEND},
  threepl_list_icon,                  /* list_icon */
  threepl_list_emblem,                                /* list_emblem */
  NULL,                /* status_text */
  NULL,               /* tooltip_text */
  threepl_status_types,               /* status_types */
  NULL,            /* blist_node_menu */
  threepl_chat_info,                  /* chat_info */
  threepl_chat_info_defaults,         /* chat_info_defaults */
  threepl_login,                      /* login */
  threepl_close,                      /* close */
  threepl_send_im,                    /* send_im */
  NULL,                   /* set_info */
  threepl_send_typing,                /* send_typing */
  threepl_get_info,                   /* get_info */
  NULL,                 /* set_status */
  NULL,                   /* set_idle */
  NULL,              /* change_passwd */
  NULL,                  /* add_buddy */
  NULL,                /* add_buddies */
  NULL,               /* remove_buddy */
  NULL,             /* remove_buddies */
  NULL,                 /* add_permit */
  NULL,                   /* add_deny */
  NULL,                 /* rem_permit */
  NULL,                   /* rem_deny */
  NULL,            /* set_permit_deny */
  threepl_join_chat,                  /* join_chat */
  NULL,                /* reject_chat */
  threepl_get_chat_name,              /* get_chat_name */
  threepl_chat_invite,                /* chat_invite */
  NULL,                 /* chat_leave */
  NULL,               /* chat_whisper */
  threepl_chat_send,                  /* chat_send */
  threepl_keepalive,                                /* keepalive */
  NULL,              /* register_user */
  NULL,                /* get_cb_info */
  NULL,                                /* get_cb_away */
  NULL,                /* alias_buddy */
  NULL,                /* group_buddy */
  NULL,               /* rename_group */
  NULL,                                /* buddy_free */
  NULL,               /* convo_closed */
  NULL,                  /* normalize */
  NULL,             /* set_buddy_icon */
  NULL,               /* remove_group */
  NULL,                                /* get_cb_real_name */
  threepl_set_chat_topic,             /* set_chat_topic */
  threepl_find_blist_chat,                                /* find_blist_chat */
  NULL,          /* roomlist_get_list */
  NULL,            /* roomlist_cancel */
  NULL,   /* roomlist_expand_category */
  threepl_can_receive_file,           /* can_receive_file */
  threepl_send_file,                                /* send_file */
  NULL,                                /* new_xfer */
  threepl_offline_message,            /* offline_message */
  NULL,                                /* whiteboard_prpl_ops */
  NULL,                                /* send_raw */
  NULL,                                /* roomlist_room_serialize */
  NULL,                                /* unregister_user */
  NULL,                                /* send_attention */
  NULL,                                /* get_attention_types */
  sizeof(PurplePluginProtocolInfo),    /* struct_size */
  threepl_get_account_text_table,                                /* get_account_text_table */
  NULL,                                /* initiate_media */
  NULL,                                /* get_media_caps */
  NULL,                                /* get_moods */
  NULL,                                /* set_public_alias */
  NULL,                                /* get_public_alias */
  threepl_add_buddy_with_invite,                                /* add_buddy_with_invite */
  NULL                                 /* add_buddies_with_invite */
};

static PurplePluginInfo threepl_plugin_info = {
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_PROTOCOL,
    NULL,
    0x00,
    NULL,
    PURPLE_PRIORITY_DEFAULT
};

static void threepl_init(PurplePlugin *plugin) {

    plugin->info = &threepl_plugin_info;

    //TODO, these must be constants (also for Command.cpp)
    plugin->info->id = strdup("prpl-threepl");
    plugin->info->name = strdup("Threema");
    plugin->info->version = strdup("0.0.1");
    plugin->info->summary = strdup("Threema protocol");
    plugin->info->description = strdup("Threema protocol plugin that can communicate with other Threema clients");
    plugin->info->homepage = strdup("http://192.168.0.1/");

    plugin->info->destroy = &threepl_destroy;
    plugin->info->extra_info = &threepl_protocol_info;

    plugin->info->actions = &threepl_actions;

    GList* opts = NULL;
    PurpleAccountOption* opt = purple_account_option_string_new("Backup string", "backup", "");
    opts = g_list_append(opts, opt);
    opt = purple_account_option_string_new("Server", "server", "g-xx.0.threema.ch");
    opts = g_list_append(opts, opt);
    opt = purple_account_option_int_new("Port", "port", 5222);
    opts = g_list_append(opts, opt);
    opt = purple_account_option_bool_new("Use application gateway", "proxy", true);
    opts = g_list_append(opts, opt);
    opt = purple_account_option_bool_new("Mark received messages as seen", "status-seen", false);
    opts = g_list_append(opts, opt);

    threepl_protocol_info.protocol_options = opts;

    threepl_register_commands(plugin);
}

static void threepl_destroy(PurplePlugin *plugin) {
    threepl_unregister_commands(plugin);

    free(plugin->info->id);
    free(plugin->info->name);
    free(plugin->info->version);
    free(plugin->info->summary);
    free(plugin->info->description);
    free(plugin->info->homepage);

    threepl::deinit_logging();
}

extern "C" gboolean purple_init_plugin(PurplePlugin *plugin) {
    threepl::init_logging();

    threepl_init(plugin);
    return purple_plugin_register(plugin);
}
