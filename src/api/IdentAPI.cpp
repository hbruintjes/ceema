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

#include "IdentAPI.h"

#include <encoding/sha256.h>
#include <encoding/base64.h>

namespace ceema {

    future<Contact> IdentAPI::getClientInfo(std::string clientid) {
        std::string url = "https://api.threema.ch/identity/" + clientid;

        future<json> responseJson = jsonGet(url);
        return responseJson.next([](future<json> res) -> Contact {
            json data = res.get();

            json id = data["identity"];
            json key = data["publicKey"];

            if (id.is_null() || key.is_null()) {
              throw std::runtime_error("Invalid data received");
            }

            client_id c_id = client_id::fromString(id);

            public_key pkey;
            if (base64_decode(key, pkey.data(), pkey.size()) != pkey.size()) {
              throw std::runtime_error("Error decoding public key");
            }

            return Contact(c_id, pkey);
        });
    }

    future<bool> IdentAPI::setFeatureLevel(Account const& client, int featureLevel) {
        std::string url = "https://api.threema.ch/identity/set_featurelevel";

        // Request featureLevel adjustment
        json requestJson;
        requestJson["identity"] = client.id().toString();
        requestJson["featureLevel"] = featureLevel;

        return jsonPost(url, requestJson).next([this, requestJson, sk=client.sk()](future<json> fut) mutable -> future<json> {
            json responseJson = fut.get();

            insertChallengeResponse(requestJson, responseJson, sk);

            std::string url = "https://api.threema.ch/identity/set_featurelevel";
            return jsonPost(url, requestJson);
        }).next([](future<json> fut) -> bool {
            json responseJson = fut.get();

            return responseJson["success"];
        });
    }
/*
    std::vector<std::string> IdentAPI::checkFeatureLevel(std::vector<Contact> const& clients) {
        std::string url = "https://api.threema.ch/identity/check_featurelevel";

        // Request featureLevel adjustment
        json requestJson;
        for (int i = 0; i < clients.size(); i++) {
            requestJson["identities"][i] = clients[i].id().toString();
        }

        json responseJson = jsonPost(url, requestJson);

        responseJson = jsonPost(url, requestJson);
        LOG_DEBUG(logging::loggerProtocol, "Result " << responseJson);

        auto levels = responseJson["featureLevels"];
        std::vector<std::string> res;
        for(int i = 0; i < levels.size(); i++) {
            res.push_back(levels[i]);
        }

        return res;
    }

    bool IdentAPI::identCheck(std::vector<Contact> const& clients) {
        std::string url = "https://api.threema.ch/identity/check";

        json requestJson;
        for(auto const& client: clients) {
            requestJson["identities"].push_back(client.id().toString());
        }

        json responseJson = jsonPost(url, requestJson);

        LOG_DEBUG(logging::loggerProtocol, "Got check data: " << responseJson);
        return true;
    }

    bool IdentAPI::checkRevocationKey(Account const& client) {
        std::string url = "https://api.threema.ch/identity/check_revocation_key";

        json requestJson;
        requestJson["identity"] = client.id().toString();

        json responseJson = jsonPost(url, requestJson);

        insertChallengeResponse(requestJson, responseJson, client.sk());

        responseJson = jsonPost(url, requestJson);
        LOG_DEBUG(logging::loggerProtocol, "Result " << responseJson);

        //Also: lastChanged, e.g. "2017-02-15T16:12:52+0000"
        return responseJson["revocationKeySet"];
    }
*/
    future<bool> IdentAPI::setRevocationKey(Account const &client, std::string const &key) {
        std::string url = "https://api.threema.ch/identity/set_revocation_key";

        auto hash = hash_sha256(reinterpret_cast<std::uint8_t const*>(key.data()), key.size());
        auto hash_slice = slice_array<0,4>(hash);

        json requestJson;
        requestJson["identity"] = client.id().toString();
        requestJson["revocationKey"] = base64_encode(hash_slice);

        return jsonPost(url, requestJson).next([this, requestJson, sk=client.sk()](future<json> fut) mutable -> future<json> {
            json responseJson = fut.get();

            insertChallengeResponse(requestJson, responseJson, sk);

            std::string url = "https://api.threema.ch/identity/set_revocation_key";
            return jsonPost(url, requestJson);
        }).next([](future<json> fut) -> bool {
            json responseJson = fut.get();

            return responseJson["revocationKeySet"];
        });
    }

    future<bool> IdentAPI::linkEmail(Account const& client, std::string email) {
        std::string url = "https://api.threema.ch/identity/link_email";

        std::string hash;
        hash.resize(crypto_hash_sha256_BYTES);
        crypto_hash_sha256((unsigned char*)hash.data(), (unsigned char const*)email.data(), email.size());

        json requestJson;
        requestJson["identity"] = client.id().toString();
        requestJson["email"] = email;

        return jsonPost(url, requestJson).next([this, requestJson, sk=client.sk()](future<json> fut) mutable -> future<json> {
            json responseJson = fut.get();

            insertChallengeResponse(requestJson, responseJson, sk);

            std::string url = "https://api.threema.ch/identity/link_email";
            return jsonPost(url, requestJson);
        }).next([](future<json> fut) -> bool {
            json responseJson = fut.get();

            return responseJson["success"];
        });
    }

    future<std::string> IdentAPI::linkMobile(Account const& client, std::string mobileNo, bool call) {
        std::string url = "https://api.threema.ch/identity/link_mobileno";

        json requestJson;
        requestJson["identity"] = client.id().toString();
        requestJson["mobileNo"] = mobileNo;
        requestJson["lang"] = "en"; //TODO: parameter ISO language

        bool threemawork = false;
        if (threemawork) {
            requestJson["urlScheme"] = true;
        }

        return jsonPost(url, requestJson).next([this, requestJson, sk=client.sk()](future<json> fut) mutable -> future<json> {
            json responseJson = fut.get();

            if (!responseJson.count("linked")) {
                throw std::runtime_error("Invalid response");
            } else if (responseJson["linked"].get<bool>()) {
                // Already done
                promise<json> p;
                p.set_value(json{"success", true});
                return p.get_future();
            }

            insertChallengeResponse(requestJson, responseJson, sk);

            std::string url = "https://api.threema.ch/identity/link_mobileno";
            return jsonPost(url, requestJson);
        }).next([](future<json> fut) -> std::string {
            json responseJson = fut.get();

            if (responseJson.count("verificationId")) {
                return responseJson["verificationId"];
            }

            // Empty string means it is already linked
            return "";
        });
    }

    future<bool> IdentAPI::callMobile(Account const& client, std::string const& verifyId) {
        std::string url = "https://api.threema.ch/identity/link_mobileno_call";

        json requestJson;
        requestJson["verificationId"] = verifyId;

        return jsonPost(url, requestJson).next([](future<json> fut) -> bool {
            json responseJson = fut.get();

            return responseJson["success"];
        });
    }

    future<bool> IdentAPI::verifyMobile(Account const& client, std::string const& verifyId, std::string const& code) {
        std::string url = "https://api.threema.ch/identity/link_mobileno";

        json requestJson;
        requestJson["verificationId"] = verifyId;
        requestJson["code"] = code;

        return jsonPost(url, requestJson).next([](future<json> fut) -> bool {
            json responseJson = fut.get();

            return responseJson["success"];
        });
    }
/*
    bool IdentAPI::checkPriv(Account const& client) {
        std::string url = "https://api.threema.ch/identity/fetch_priv";

        json requestJson;
        requestJson["identity"] = client.id().toString();

        json responseJson = jsonPost(url, requestJson);

        insertChallengeResponse(requestJson, responseJson, client.sk());

        responseJson = jsonPost(url, requestJson);
        LOG_DEBUG(logging::loggerProtocol, "Result " << responseJson);
    }

    Account IdentAPI::createIdentity(std::string const& license) {
        std::string url = "https://api.threema.ch/identity/create";

        public_key pk;
        private_key sk;
        if (!crypto::generate_keypair(pk, sk)) {
            throw std::runtime_error("Unable to generate keypair");
        }

        LOG_DEBUG(logging::loggerRoot, "Generate ident");
        LOG_DEBUG(logging::loggerRoot, "SK: " << sk);
        LOG_DEBUG(logging::loggerRoot, "PK: " << pk);

        json requestJson;
        requestJson["publicKey"] = base64_encode(pk);

        json responseJson = jsonPost(url, requestJson);

        LOG_DEBUG(logging::loggerRoot, "Response: " << responseJson);

        insertChallengeResponse(requestJson, responseJson, sk, createIdentNonce);

        requestJson.erase("nonce");
        requestJson["licenseKey"] = license;
        // Note: Not supporting licenseUsername and licensePassword fields
        // for business accounts

        LOG_DEBUG(logging::loggerRoot, "Request 2: " << requestJson);

        responseJson = jsonPost(url, requestJson);

        LOG_DEBUG(logging::loggerRoot, "Response 2: " << responseJson);

        if (responseJson["identity"].is_null()) {
            throw std::runtime_error("Could not create ident");
        }

        return Account(client_id::fromString(responseJson["identity"]), sk);
    }

    bool IdentAPI::checkVersionStatus() {
        std::string url = "https://api.threema.ch/check_license";

        json requestJson;
        requestJson["license"] = "";
        requestJson["deviceId"] = "";
        requestJson["version"] = "0.2;J;;;1.7.0";

        json responseJson = jsonPost(url, requestJson);

        LOG_DEBUG(logging::loggerRoot, "Response: " << responseJson);

        return true;
    }

    bool IdentAPI::match(std::unordered_map<std::string, Contact>& phone, std::unordered_map<std::string, Contact>& mail) {
        std::string url = "https://api.threema.ch/identity/match";

        std::unordered_map<std::string, std::string> phoneHashes;
        std::unordered_map<std::string, std::string> mailHashes;

        json phoneArray, mailArray;


        for(auto entry: phone) {
            auto hash = hash_phone(entry.first);
            phoneHashes[hash] = entry.first;
            phoneArray.push_back(hash);
        }

        for(auto entry: mail) {
            //NOTE: not doing the gmail == googlemail crap
            auto hash = hash_mail(entry.first);
            mailHashes[hash] = entry.first;
            mailArray.push_back(hash);
        }

        json requestJson;
        requestJson["emailHashes"] = mailArray;
        requestJson["mobileNoHashes"] = phoneArray;

        json responseJson = jsonPost(url, requestJson);
        LOG_DEBUG(logging::loggerProtocol, "Result " << responseJson);

        auto entries = responseJson["identities"].size();
        for(std::size_t i = 0; i < entries; i++) {
            auto ident = responseJson["identities"][i];

            client_id id = client_id::fromString(ident["identity"]);
            public_key publicKey;
            base64_decode(ident["publicKey"], publicKey.data(), publicKey.size());
            Contact c(id, publicKey);
            if (!ident["emailHash"].is_null()) {
                auto email = mailHashes[ident["emailHash"]];
                mail.at(email) = c;
            } else if (!ident["mobileNoHash"].is_null()) {
                auto phonenr = phoneHashes[ident["mobileNoHash"]];
                phone.at(phonenr) = c;
            } // else corrupt, ignore
        }

        return true;
    }
*/
    bool IdentAPI::insertChallengeResponse(json& request,
                                           json const& challenge,
                                           private_key const& sk,
                                           nonce const& n) {
        // Confirm request by encrypting token with client SK
        public_key respPK;
        byte_vector token;

        base64_decode(challenge["tokenRespKeyPub"], respPK.data(), respPK.size());
        token = base64_decode(challenge["token"]);

        token.resize(token.size() + crypto_box_MACBYTES);
        if (!crypto::box::encrypt_inplace(token, n, respPK, sk)) {
            return false;
        }

        std::string base64_nonce = base64_encode(n);
        std::string base64_response = base64_encode(token);

        request["token"] = challenge["token"];
        request["nonce"] = base64_nonce;
        request["response"] = base64_response;

        return true;
    }

    std::string IdentAPI::hash_phone(std::string phone) {
        //TODO: enforce standarized format
        auto hash = mac_sha256(reinterpret_cast<const std::uint8_t*>(phone.data()), phone.size(), phoneKey.data(), phoneKey.size());
        return base64_encode(hash);
    }

    std::string IdentAPI::hash_mail(std::string mail) {
        //TODO: trim & tolower
        auto hash = mac_sha256(reinterpret_cast<const std::uint8_t*>(mail.data()), mail.size(), emailKey.data(), emailKey.size());
        return base64_encode(hash);
    }

    byte_array<32> IdentAPI::emailKey{
            0x30, 0xa5, 0x50, 0x0f, 0xed, 0x97, 0x01, 0xfa,
            0x6d, 0xef, 0xdb, 0x61, 0x08, 0x41, 0x90, 0x0f,
            0xeb, 0xb8, 0xe4, 0x30, 0x88, 0x1f, 0x7a, 0xd8,
            0x16, 0x82, 0x62, 0x64, 0xec, 0x09, 0xba, 0xd7 };

    byte_array<32> IdentAPI::phoneKey{
            0x85, 0xad, 0xf8, 0x22, 0x69, 0x53, 0xf3, 0xd9,
            0x6c, 0xfd, 0x5d, 0x09, 0xbf, 0x29, 0x55, 0x5e,
            0xb9, 0x55, 0xfc, 0xd8, 0xaa, 0x5e, 0xc4, 0xf9,
            0xfc, 0xd8, 0x69, 0xe2, 0x58, 0x37, 0x07, 0x23 };

    nonce IdentAPI::createIdentNonce{
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
}
