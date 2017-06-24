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

#include <encoding/crypto.h>
#include <protocol/data/Client.h>

namespace ceema {

    /**
     * Identifies a client, which can be the local user (has secret key), or some contact (only public key)
     */
    class Contact {
        // Long term client keypair
        public_key m_clientPK;

        // Contact ID
        client_id m_clientID;
        std::string m_nick;
    public:
        Contact(client_id const &id, public_key const &pk) : m_clientPK(pk), m_clientID(id), m_nick(id.toString()) {
        }

        client_id const &id() const {
            return m_clientID;
        }

        std::string const &nick() const {
            return m_nick;
        }

        public_key const &pk() const {
            return m_clientPK;
        }


    };

}
