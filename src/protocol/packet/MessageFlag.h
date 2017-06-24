//
// Created by harold on 19/05/17.
//

#ifndef CEEMA_MESSAGEFLAG_H_H
#define CEEMA_MESSAGEFLAG_H_H

#include <types/flags.h>

#include <cstdint>
#include <iosfwd>

namespace ceema {
    enum class MessageFlag {
        /** Send as push message */
                PUSH = 0x01,
        /** Do not queue message when recipient offline */
                NO_QUEUE = 0x02,
        /** Do not ACK message by server */
                NO_ACK = 0x04,
        /** Notify recipient when message arrives */
                NOTIFY = 0x08,
        /** Message is a group message (has group payload) */
                GROUP = 0x10,
    };
    typedef Flags<MessageFlag, std::uint32_t> MessageFlags;

    std::ostream& operator<<(std::ostream& os, MessageFlag flag);
}
#endif //CEEMA_MESSAGEFLAG_H_H
