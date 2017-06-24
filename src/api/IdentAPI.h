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

#include "api/API.h"

#include <contact/Contact.h>
#include <unordered_map>
#include <contact/Account.h>

namespace ceema {

    class IdentAPI : public API {
        // E-Mail addresses are hashed with fixed key
        static byte_array<32> emailKey;
        // Phone nos. are hashed with fixed key
        static byte_array<32> phoneKey;
        static nonce createIdentNonce;

    public:
        explicit IdentAPI(HttpManager& manager) : API(manager) {}

        future<Contact> getClientInfo(std::string client_id);

        future<bool> setFeatureLevel(Account const& client, int featureLevel);
/*
        std::vector<std::string> checkFeatureLevel(std::vector<Contact> const& clients);

        bool identCheck(std::vector<Contact> const& clients);

        bool checkRevocationKey(Account const& client);
*/
        future<bool> setRevocationKey(Account const &client, std::string const &key);

        future<bool> linkEmail(Account const& client, std::string email);

        future<std::string> linkMobile(Account const& client, std::string mobileNo, bool call);

        future<bool> callMobile(Account const& client, std::string const& verifyId);

        future<bool> verifyMobile(Account const& client, std::string const& verifyId, std::string const& code);
/*
        bool checkPriv(Account const& client);

        Account createIdentity(std::string const& license);

        bool checkVersionStatus();

        bool match(std::unordered_map<std::string, Contact>& phone, std::unordered_map<std::string, Contact>& mail);
        */
    private:
        bool insertChallengeResponse(json& request, json const& challenge, private_key const& sk, nonce const& n = crypto::generate_nonce());

        std::string hash_phone(std::string phone);
        std::string hash_mail(std::string mail);


    };
}
