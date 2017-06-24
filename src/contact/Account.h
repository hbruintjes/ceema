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

#include "Contact.h"

namespace ceema {

    class Account : public Contact {
        private_key m_clientSK;

    public:
        Account(client_id const &id, private_key const &sk) : Contact(id, crypto::derive_public_key(sk)),
                                                              m_clientSK(sk) {

        }

        private_key const &sk() const {
            return m_clientSK;
        }
    };

}
