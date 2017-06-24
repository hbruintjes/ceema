//
// Created by harold on 30/05/17.
//

#ifndef CEEMA_XFER_H
#define CEEMA_XFER_H

#include <libpurple/connection.h>

gboolean threepl_can_receive_file(PurpleConnection *, const char *who);

void threepl_send_file(PurpleConnection* gc, const char *who, const char *filename);

#endif //CEEMA_XFER_H
