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

#include <protocol/data/Crypto.h>

namespace ceema {

    namespace crypto {
        class crypto_error : public std::runtime_error {
            using std::runtime_error::runtime_error;
        };

        extern int sodium_status;

        template<typename Array>
        bool randombytes(Array& array) {
            randombytes_buf(array.data(), array.size());
            return true;
        }

        namespace random {
            inline std::uint32_t random() {
                return randombytes_random();
            }
        }


        inline bool generate_keypair(public_key& pk, private_key& sk) {
            return crypto_box_keypair(pk.data(), sk.data()) == 0;
        }

        inline bool generate_shared_key(shared_key& k) {
            return randombytes(k);
        }

        inline shared_key generate_shared_key() {
            shared_key k;
            randombytes(k);
            return k;
        }

        inline public_key derive_public_key(private_key const& sk) {
            public_key pk;
            crypto_scalarmult_base(pk.data(), sk.data());
            return pk;
        }

        inline nonce generate_nonce() {
            nonce n;
            randombytes_buf(n.data(), n.size());
            return n;
        }

        namespace box {
            template<typename ArrayOut, typename ArrayIn>
            bool encrypt(ArrayOut& result, ArrayIn& input, nonce const& n, public_key const& pk, private_key const& sk) {
                if (result.size() != (input.size() + crypto_box_MACBYTES)) {
                    throw crypto_error("Invalid output size");
                }
                return crypto_box_easy(result.data(), input.data(), input.size(),
                                       n.data(), pk.data(), sk.data()) == 0;
            }

            template<typename Array>
            bool encrypt_inplace(Array& data, nonce const& n, public_key const& pk, private_key const& sk) {
                if (data.size() < crypto_box_MACBYTES) {
                    throw crypto_error("Invalid buffer size");
                }
                return crypto_box_easy(data.data(), data.data(), data.size() - crypto_box_MACBYTES,
                                       n.data(), pk.data(), sk.data()) == 0;
            }

            template<typename ArrayOut, typename ArrayIn>
            bool decrypt(ArrayOut& result, ArrayIn& input, nonce const& n, public_key const& pk, private_key const& sk) {
                if (input.size() != (result.size() + crypto_box_MACBYTES)) {
                    throw crypto_error("Invalid output size");
                }
                return crypto_box_open_easy(result.data(), input.data(), input.size(),
                                            n.data(), pk.data(), sk.data()) == 0;
            }

            template<typename Array>
            bool decrypt_inplace(Array& data, nonce const& n, public_key const& pk, private_key const& sk) {
                if (data.size() < crypto_box_MACBYTES) {
                    throw crypto_error("Invalid buffer size");
                }
                return crypto_box_open_easy(data.data(), data.data(), data.size(),
                                            n.data(), pk.data(), sk.data()) == 0;
            }

            template<typename Array>
            bool decrypt_inplace(Array& data, std::size_t size, nonce const& n, public_key const& pk, private_key const& sk) {
                if (data.size() < crypto_box_MACBYTES) {
                    throw crypto_error("Invalid buffer size");
                }
                return crypto_box_open_easy(data.data(), data.data(), size,
                                            n.data(), pk.data(), sk.data()) == 0;
            }

        }

        namespace secretbox {

            template<typename ArrayOut, typename ArrayIn>
            bool encrypt(ArrayOut& result, ArrayIn& input, nonce const& n, shared_key const& k) {
                if (result.size() != (input.size() + crypto_secretbox_MACBYTES)) {
                    throw crypto_error("Invalid output size");
                }
                return crypto_secretbox_easy(result.data(), input.data(), input.size(),
                                             n.data(), k.data()) == 0;
            }

            template<typename Array>
            bool encrypt_inplace(Array& data, nonce const& n, shared_key const& k) {
                if (data.size() < crypto_secretbox_MACBYTES) {
                    throw crypto_error("Invalid buffer size");
                }
                return crypto_secretbox_easy(data.data(), data.data(), data.size() - crypto_secretbox_MACBYTES,
                                             n.data(), k.data()) == 0;
            }

            template<typename ArrayOut, typename ArrayIn>
            bool decrypt(ArrayOut& result, ArrayIn& input, nonce const& n, shared_key const& k) {
                if (input.size() != (result.size() + crypto_secretbox_MACBYTES)) {
                    throw crypto_error("Invalid output size");
                }
                return crypto_secretbox_open_easy(result.data(), input.data(), input.size(),
                                                  n.data(), k.data()) == 0;
            }

            template<typename Array>
            bool decrypt_inplace(Array& data, nonce const& n, shared_key const& k) {
                if (data.size() < crypto_secretbox_MACBYTES) {
                    throw crypto_error("Invalid buffer size");
                }
                return crypto_secretbox_open_easy(data.data(), data.data(), data.size(),
                                                  n.data(), k.data()) == 0;
            }
        }

    }

}
