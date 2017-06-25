//
// Created by harold on 25/06/17.
//

#ifndef CEEMA_FORMATSTR_H
#define CEEMA_FORMATSTR_H

#include <sstream>

class formatstr: private std::stringstream {
public:
    using std::stringstream::stringstream;

    operator std::string() const {
        return str();
    }

    using std::stringstream::str;

    template<typename T>
    formatstr& operator<<(T const& t) {
        static_cast<std::stringstream&>(*this) << t;
        return *this;
    }
};

#endif //CEEMA_FORMATSTR_H
