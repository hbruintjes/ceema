//
// Created by harold on 18-3-17.
//

#ifndef CEEMA_ACTIONS_H
#define CEEMA_ACTIONS_H

#include <libpurple/plugin.h>

GList* threepl_actions(PurplePlugin *plugin, gpointer context);

// typedef required for more complex actions like import from backup
// added by Torsten (ttl)
typedef struct {
  PurpleConnection*  gc;
  ThreeplConnection* connection;
  char               text[2048];
} threepl_actions_data;

#endif //CEEMA_ACTIONS_H
