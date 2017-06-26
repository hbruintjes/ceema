//
// Created by harold on 26/06/17.
//

#ifndef CEEMA_BUDDY_H
#define CEEMA_BUDDY_H


#include <contact/Contact.h>
#include <libpurple/blist.h>
#include <protocol/packet/message_id.h>

/**
 * Class to hold buddy data
 */
class Buddy {
    ceema::Contact m_contact;
    PurpleBuddy* m_buddy;

    ceema::message_id m_lastMessage;
    std::string m_nickname;
    unsigned int m_featureLevel;
    unsigned int m_featureMask;
    time_t m_icon_ts;
    time_t m_update_ts;
public:

};


#endif //CEEMA_BUDDY_H
