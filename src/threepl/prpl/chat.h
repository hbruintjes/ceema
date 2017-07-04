//
// Created by harold on 30/05/17.
//

#ifndef CEEMA_CHAT_H
#define CEEMA_CHAT_H

#include <libpurple/conversation.h>

/**
 * Get the name of a chat as used for running conversations,
 * or creating new ones
 * @param components Chat components
 * @return glib allocated string, caller must g_free
 */
char* threepl_get_chat_name(GHashTable *components);

/**
 * Instruct threepl to join a chat based on provided chat info. Triggers
 * a server_got_join
 * @param gc
 * @param components
 */
void threepl_join_chat(PurpleConnection *gc, GHashTable* components);

/*
static void threepl_reject_chat(PurpleConnection *gc, GHashTable *components);

static void threepl_chat_leave(PurpleConnection *gc, int id);
*/

GList* threepl_chat_info(PurpleConnection* gc);

/**
 * Default values (generates chat ID automatically), or pulls it out
 * of room (as formatted using threepl_get_chat_name)
 * @param gc
 * @param room
 * @return
 */
GHashTable* threepl_chat_info_defaults(PurpleConnection* gc, const char* room);

PurpleChat* threepl_find_blist_chat(PurpleAccount* account, const char* name);

void threepl_chat_invite(PurpleConnection* gc, int id, const char* message, const char* who);

int threepl_chat_send(PurpleConnection* gc, int id, const char* message, PurpleMessageFlags flags);

void threepl_set_chat_topic(PurpleConnection *gc, int id, const char *topic);

#endif //CEEMA_CHAT_H
