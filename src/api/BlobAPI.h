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

#include "API.h"
#include "protocol/data/Blob.h"
#include "encoding/hex.h"

namespace ceema {

    class IBlobUploadTransfer : public IHttpTransfer {
        byte_vector m_writeBuffer;
        promise<Blob> m_promise;

    protected:
        Blob m_blob;

    public:
        IBlobUploadTransfer() {
            crypto::generate_shared_key(m_blob.key);
            m_blob.size = 0;
        }

        future<Blob> get_future() {
            return m_promise.get_future();
        }

        ssize_t write(const unsigned char* buffer, std::size_t available) override {
            m_writeBuffer.reserve(m_writeBuffer.size() + available);
            m_writeBuffer.insert(m_writeBuffer.end(), buffer, buffer+available);
            return available;
        }

        void onComplete() override {
            try {
                hex_decode(m_writeBuffer, m_blob.id);
                m_promise.set_value(std::move(m_blob));
            } catch (std::exception& e) {
                m_promise.set_exception(std::make_exception_ptr(std::runtime_error("Received invalid blob id")));
            }
        }

        void onFailed(CURLcode errCode, const char* errMsg) override {
            m_promise.set_exception(std::make_exception_ptr(std::runtime_error(errMsg)));
        }
    };

    class BlobAPI : public API {
        std::string m_url;
        bool m_useTLS;

        static nonce nonceVideo;
        static nonce nonceVideoThumb;
        static nonce nonceAudio;
        static nonce nonceFile;
        static nonce nonceFileThumb;
        static nonce nonceGroupImage;
        static nonce nonceGroupIcon;
        static nonce nonceIcon;

    public:
        BlobAPI(HttpManager& manager, bool useTLS);

        void upload(IHttpTransfer *transfer);

        future<LegacyBlob> uploadImage(std::string const& fileName, public_key const& pk, private_key const& sk);

        future<void> downloadFile(IHttpTransfer* transfer, blob_id const& id);

        future<void> downloadImage(std::string const& fileName, LegacyBlob const& blob, public_key const& pk, private_key const& sk);

        future<void> deleteBlob(blob_id const& id);

        template<typename ArrayIn, typename ArrayOut>
        static bool encrypt(ArrayOut& result, ArrayIn& input, BlobType type, shared_key key) {
            result.resize(input.size() + crypto_secretbox_MACBYTES);
            return crypto::secretbox::encrypt(result, input, BlobAPI::getFixedNonce(type), key);
        }

        template<typename Array>
        static bool encrypt(Array& data, BlobType type, shared_key key) {
            data.resize(data.size() + crypto_secretbox_MACBYTES);
            return crypto::secretbox::encrypt_inplace(data, BlobAPI::getFixedNonce(type), key);
        }

        template<typename Array>
        static bool encrypt(Array& data, nonce n, public_key const& pk, private_key const& sk) {
            data.resize(data.size() + crypto_box_MACBYTES);
            return crypto::box::encrypt_inplace(data, n, pk, sk);
        }

        template<typename ArrayIn, typename ArrayOut>
        static bool decrypt(ArrayOut& result, ArrayIn& input, BlobType type, shared_key key) {
            result.resize(input.size() - crypto_secretbox_MACBYTES);
            return crypto::secretbox::decrypt(result, input, BlobAPI::getFixedNonce(type), key);
        }

        template<typename Array>
        static bool decrypt(Array& data, BlobType type, shared_key key) {
            bool decrypt_ok = crypto::secretbox::decrypt_inplace(data, BlobAPI::getFixedNonce(type), key);
            data.resize(data.size() - crypto_secretbox_MACBYTES);
            return decrypt_ok;
        }

        template<typename Array>
        static bool decrypt(Array& data, nonce n, public_key const& pk, private_key const& sk) {
            bool decrypt_ok = crypto::box::decrypt_inplace(data, n, pk, sk);
            data.resize(data.size() - crypto_box_MACBYTES);
            return decrypt_ok;
        }

        static nonce const& getFixedNonce(BlobType type);

        static std::string getDownloadURL(blob_id const& id, bool useTLS = true);

    private:
        future<LegacyBlob> uploadData(byte_vector& data, BlobType type, public_key const& pk, private_key const& sk);
        future<void> downloadData(IHttpTransfer* transfer, blob_id const& id);
        future<byte_vector> downloadData(LegacyBlob const& blob, public_key const& pk, private_key const& sk);

    };

}
