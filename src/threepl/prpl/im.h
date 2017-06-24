//
// Created by harold on 30/05/17.
//

#ifndef CEEMA_IM_H
#define CEEMA_IM_H

#include <libpurple/connection.h>
#include <libpurple/conversation.h>

int threepl_send_im(PurpleConnection* gc, const char *who, const char *message, PurpleMessageFlags);

unsigned int threepl_send_typing(PurpleConnection* gc, const char* who, PurpleTypingState state);

#endif //CEEMA_IM_H
