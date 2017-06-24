//
// Created by harold on 30/05/17.
//

#ifndef CEEMA_LIST_H
#define CEEMA_LIST_H

typedef struct _PurpleAccount PurpleAccount;
typedef struct _PurpleBuddy PurpleBuddy;

const char* threepl_list_icon(PurpleAccount *acct, PurpleBuddy *buddy);

const char* threepl_list_emblem(PurpleBuddy* buddy);

#endif //CEEMA_LIST_H
