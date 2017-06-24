//
// Created by harold on 20/05/17.
//

#ifndef CEEMA_COMMANDS_H
#define CEEMA_COMMANDS_H

#include <libpurple/plugin.h>
#include <libpurple/cmds.h>

void threepl_register_commands(PurplePlugin *plugin);
void threepl_unregister_commands(PurplePlugin *plugin);

PurpleCmdRet threepl_message_agree(PurpleConversation *, const gchar *cmd,
                                   gchar **args, gchar **error, void *data);

#endif //CEEMA_COMMANDS_H
