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

#include "HttpClient.h"
#include "BlobAPI.h"

namespace ceema {

    class TransferWrapper : public IHttpTransfer {
    protected:
        HttpBufferTransfer m_transfer;

    public:
        TransferWrapper() = default;
        TransferWrapper(byte_vector data) : m_transfer(std::move(data)) {}

        void onStart() override {
            m_transfer.onStart();
        }

        ssize_t write(const unsigned char* buffer, std::size_t available) override {
            return m_transfer.write(buffer, available);
        }

        ssize_t read(unsigned char* buffer, std::size_t expected) override {
            return m_transfer.read(buffer, expected);
        }

        void onComplete() override {
            m_transfer.onComplete();
        }

        void onFailed(CURLcode errCode, const char* errMsg) override {
            m_transfer.onFailed(errCode, errMsg);
        }

        ssize_t size() override {
            return m_transfer.size();
        }

        bool cancelled() override {
            return m_transfer.cancelled();
        }
    };

    class BlobUploadTransfer : public TransferWrapper {
        BlobType m_type;
        shared_key m_key;
        std::uint32_t m_size;

    public:
        BlobUploadTransfer(byte_vector data, BlobType type) :
                BlobUploadTransfer(data, type, crypto::generate_shared_key()) {
        }

        BlobUploadTransfer(byte_vector data, BlobType type, shared_key key) :
                m_type(type), m_key(key) {
            m_size = data.size();
            LOG_DBG("Encrypt blob using " << m_key);
            BlobAPI::encrypt(data, type, m_key);
            m_transfer.set_buffer(std::move(data));
        }

        future<Blob> get_future() {
            return m_transfer.get_future().next([this](future<byte_vector> fut) {
                Blob blob;
                byte_vector data = fut.get();
                hex_decode(data, blob.id);

                blob.key = m_key;
                blob.size = m_size;

                return blob;
            });
        }

    };

    class LegacyBlobUploadTransfer : public TransferWrapper {
        nonce m_nonce;
        std::uint32_t m_size;

    public:
        LegacyBlobUploadTransfer(byte_vector data, public_key pk, private_key sk) :
                LegacyBlobUploadTransfer(data, pk, sk, crypto::generate_nonce()) {}

        LegacyBlobUploadTransfer(byte_vector data, public_key pk, private_key sk, nonce n) :
                m_nonce(n) {
            m_size = data.size();
            BlobAPI::encrypt(data, m_nonce, pk, sk);
            m_transfer.set_buffer(std::move(data));
        }

        future<LegacyBlob> get_future() {
            return m_transfer.get_future().next([this](future<byte_vector> fut) {
                LegacyBlob blob;
                byte_vector data = fut.get();
                hex_decode(data, blob.id);

                blob.n = m_nonce;
                blob.size = m_size;

                return blob;
            });
        }
    };

    class BlobDownloadTransfer : public TransferWrapper {
        blob_id m_id;
        BlobType m_type;
        shared_key m_key;

    public:
        BlobDownloadTransfer(blob_id id, BlobType type, shared_key key) :
                m_id(id), m_type(type), m_key(key) {
        }

        future<byte_vector> get_future() {
            return m_transfer.get_future().next([this](future<byte_vector> fut) {
                byte_vector data = fut.get();
                LOG_DBG("Decrypt blob using " << m_key);
                if (!BlobAPI::decrypt(data, m_type, m_key)) {
                    throw std::runtime_error("Unable to decrypt data");
                }
                return data;
            });
        }
    };

    class LegacyBlobDownloadTransfer : public TransferWrapper {
        blob_id m_id;
        nonce m_nonce;
        public_key m_pk;
        private_key m_sk;

    public:
        LegacyBlobDownloadTransfer(blob_id id, nonce n, public_key pk, private_key sk) :
                m_id(id), m_nonce(n), m_pk(pk), m_sk(sk) {
        }

        future<byte_vector> get_future() {
            return m_transfer.get_future().next([this](future<byte_vector> fut) {
                byte_vector data = fut.get();
                if (!BlobAPI::decrypt(data, m_nonce, m_pk, m_sk)) {
                    throw std::runtime_error("Unable to decrypt data");
                }
                return data;
            });
        }

    };
}
