//
// Created by harold on 19/05/17.
//

#include "MessageFlag.h"

#include <ostream>

namespace ceema {
    std::ostream& operator<<(std::ostream& os, MessageFlag flag) {
        switch(flag) {
            case MessageFlag::PUSH:
                return os << "PUSH";
            case MessageFlag::NO_QUEUE:
                return os << "NO_QUEUE";
            case MessageFlag::NO_ACK:
                return os << "NO_ACK";
            case MessageFlag::NOTIFY:
                return os << "NOTIFY";
            case MessageFlag::GROUP:
                return os << "GROUP";
        }
        throw std::domain_error("");
    }
}