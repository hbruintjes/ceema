//
// Created by harold on 30/05/17.
//

#ifndef CEEMA_CHAT_H
#define CEEMA_CHAT_H

#include <libpurple/conversation.h>

char* threepl_get_chat_name(GHashTable *components);

void threepl_join_chat(PurpleConnection *gc, GHashTable* components);

/*
static void threepl_reject_chat(PurpleConnection *gc, GHashTable *components);

static void threepl_chat_leave(PurpleConnection *gc, int id);
*/

GList* threepl_chat_info(PurpleConnection* gc);

GHashTable* threepl_chat_info_defaults(PurpleConnection* gc, const char* room);

PurpleChat* threepl_find_blist_chat(PurpleAccount* account, const char* name);

void threepl_chat_invite(PurpleConnection* gc, int id, const char* message, const char* who);

int threepl_chat_send(PurpleConnection* gc, int id, const char* message, PurpleMessageFlags flags);

#endif //CEEMA_CHAT_H
