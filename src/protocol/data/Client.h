/**
 * Copyright 2017 Harold Bruintjes
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "types/bytes.h"

namespace ceema {

    /** 8 character client ID (e.g. ABCDEFGH or *THREEMA) */
    const unsigned CLIENTID_SIZE = 8;
    class client_id : public byte_array<CLIENTID_SIZE> {
    public:
        using byte_array::byte_array;

        bool operator==(client_id const& other) const {
            return std::equal(begin(), end(), other.begin());
        }
        bool operator!=(client_id const& other) const {
            return !(*this == other);
        }

        std::string toString() const {
            return std::string(reinterpret_cast<const char*>(this->data()), CLIENTID_SIZE);
        }

        /**
         * Create client_id from a string. Converts to upper case
         * using current locale. Input should match [*A-Za-z0-9][A-Za-z0-9]{7}
         * but is not enforced (it most likely will result in failure to
         * find PK)
         * @param str client id as string
         * @return client_id
         * @throws runtime_error if str is of the wrong length
         */
        static client_id fromString(std::string const& str) {
            if (str.size() != CLIENTID_SIZE) {
                throw std::runtime_error("Invalid client id");
            }

            client_id id;
            auto iter = id.begin();
            for(auto c: str) {
                // Convert to upper case for ease of use
                *iter = static_cast<std::uint8_t>(toupper(c));
            }
            return id;
        }
    };

}

namespace std {
    template<> struct hash<ceema::client_id>
    {
        typedef ceema::client_id argument_type;
        typedef std::size_t result_type;
        result_type operator()(argument_type const& s) const
        {
            return std::hash<std::uint64_t>{}(*reinterpret_cast<std::uint64_t const*>(s.data())) ;
        }
    };
}
