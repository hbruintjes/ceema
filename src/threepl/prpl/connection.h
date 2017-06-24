//
// Created by harold on 30/05/17.
//

#ifndef CEEMA_CONNECTION_H
#define CEEMA_CONNECTION_H

#include <libpurple/account.h>
#include <libpurple/connection.h>

void threepl_login(PurpleAccount *account);

void threepl_close(PurpleConnection *gc);

void threepl_keepalive(PurpleConnection* gc);

#endif //CEEMA_CONNECTION_H
