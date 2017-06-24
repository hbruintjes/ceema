//
// Created by harold on 30/05/17.
//

#include "list.h"

#include <libpurple/blist.h>

const char* threepl_list_icon(PurpleAccount *acct, PurpleBuddy *buddy) {
    return "threema";
}

const char* threepl_list_emblem(PurpleBuddy* buddy)  {
    const char* pk = purple_blist_node_get_string(&buddy->node, "public-key");
    if (pk && pk[0] == '!') {
        return "not-authorized"; // Public key not found
    }
    if (purple_buddy_get_name(buddy)[0] == '*') {
        return "bot"; // Gateway account (starts with *)
    }
    return NULL;
}