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

#include <string>
#include <curl/curl.h>
#include <stdexcept>
#include <array>
#include <types/bytes.h>
#include <logging/logging.h>
#include <json.hpp>

#include <async/future.h>

typedef struct x509_st X509;

using json = nlohmann::json;

namespace ceema {
    class HttpManager;

    class IHttpTransfer {
    public:
        /**
         * Called when the transfer is initiated
         */
        virtual void onStart() {}

        /**
         * Called when more data can be uploaded.
         * @param buffer Buffer to write to
         * @param expected Size available in buffer
         * @return Number of bytes provided, 0 on EOF, -1 on error
         */
        virtual ssize_t read(unsigned char* buffer, std::size_t expected) {
            return 0;
        }

        /**
         *
         * @param buffer Data to download
         * @param available Bytes available in buffer
         * @return Number of bytes read, 0 on EOF, -1 on error
         */
        virtual ssize_t write(const unsigned char* buffer, std::size_t available) {
            return available;
        }

        /**
         * Call when the transfer has successfully completed
         */
        virtual void onComplete() {
        }

        /**
         * Call when the transfer has failed
         */
        virtual void onFailed(CURLcode errCode, const char* errMsg) {
        }

        /**
         * Checks if transfer is to be cancelled
         * @return True if transfer should be canceled (triggers onFailed if not completed)
         */
        virtual bool cancelled() {
            return false;
        }

        /**
         * Returns the size of the transfer, or -1 if not known
         * @return
         */
        virtual ssize_t size() {
            return -1;
        }
    };

    template<typename T = void>
    class FutureHttpTransfer: public IHttpTransfer {
    protected:
        promise<T> m_promise;

    public:
        future<T> get_future() {
            return m_promise.get_future();
        }

        void onComplete() override {
            m_promise.set_value(get_value());
        }

        void onFailed(CURLcode errCode, const char* errMsg) override {
            std::string err = curl_easy_strerror(errCode);
            if (errMsg[0]) {
                err += ": ";
                err += errMsg;
            }
            m_promise.set_exception(std::make_exception_ptr(std::runtime_error(err)));
        }

    protected:
        virtual T&& get_value() = 0;
    };

    template<>
    class FutureHttpTransfer<void>: public IHttpTransfer {
    protected:
        promise<void> m_promise;

    public:
        future<void> get_future() {
            return m_promise.get_future();
        }

        void onComplete() override {
            m_promise.set_value();
        }

        void onFailed(CURLcode errCode, const char* errMsg) override {
            std::string err = curl_easy_strerror(errCode);
            if (errMsg[0]) {
                err += ": ";
                err += errMsg;
            }
            m_promise.set_exception(std::make_exception_ptr(std::runtime_error(err)));
        }
    };

    class HttpBufferTransfer: public FutureHttpTransfer<byte_vector> {
        byte_vector m_readBuffer;
        byte_vector::iterator m_readIter;

        byte_vector m_writeBuffer;

    public:
        HttpBufferTransfer() {
        }

        HttpBufferTransfer(byte_vector buffer) : m_readBuffer(buffer) {
        }

        void set_buffer(byte_vector buffer) {
            m_readBuffer = buffer;
        }

        void onStart() override {
            m_readIter = m_readBuffer.begin();
            m_writeBuffer.clear();
        }

        ssize_t read(unsigned char* buffer, std::size_t expected) override {
            if (m_readIter != m_readBuffer.end()) {
                auto max_len = std::min(expected, static_cast<std::size_t>(m_readBuffer.end() - m_readIter));
                std::copy(m_readIter, m_readIter+max_len, buffer);
                m_readIter += max_len;
                return max_len;
            } else {
                return 0;
            }
        }

        ssize_t write(const unsigned char* buffer, std::size_t available) override {
            m_writeBuffer.reserve(m_writeBuffer.size() + available);
            m_writeBuffer.insert(m_writeBuffer.end(), buffer, buffer+available);
            return available;
        }

        void onComplete() override {
            m_readBuffer.clear();
            FutureHttpTransfer::onComplete();
        }

        void onFailed(CURLcode errCode, const char* errMsg) override {
            m_readBuffer.clear();
            m_writeBuffer.clear();
            FutureHttpTransfer::onFailed(errCode, errMsg);
        }

        ssize_t size() override {
            return m_readBuffer.size();
        }


    protected:
        byte_vector&& get_value() override {
            return std::move(m_writeBuffer);
        }
    };


    class HttpClient {
        CURL * m_curl;
        std::array<char, CURL_ERROR_SIZE> m_errbuf;
        X509* m_cert;
        CURLcode m_lastRes;

        std::string m_userAgent;

        HttpBufferTransfer m_bufferedTransfer;

        IHttpTransfer* m_transfer;

        bool m_busy;

        HttpManager& m_manager;

        // Allows to perform cleanup of CURL resources when done
        promise<void> m_currentTask;
    public:
        HttpClient(HttpManager& manager, std::string const& user_agent) : m_curl(NULL), m_errbuf{}, m_cert(NULL), m_lastRes(CURLE_OK),
                                                                          m_userAgent(user_agent), m_transfer(nullptr),
                                                                          m_busy(false), m_manager(manager) {
            init();
        }

        ~HttpClient();

        CURL* getCURL() const {
            return m_curl;
        }

        bool getBusy() const {
            return m_busy;
        }

        void set_cert(X509* cert);

        /**
         * GET url
         * @param url URL to retrieve
         * @return Future holding response data
         */
        future<byte_vector> get(std::string const& url);

        future<void> get(std::string const& url, IHttpTransfer* transfer);

        future<byte_vector> post(std::string url, byte_vector data);

        void post(std::string url, IHttpTransfer* transfer);

        future<byte_vector> postFile(std::string url, byte_vector data, std::string const& filename);

        void postFile(std::string url, IHttpTransfer* transfer, std::string const& filename);

        /**
         * Close any open connections
         */
        void close() {
            curl_easy_cleanup(m_curl);
            m_curl = NULL;
        }

        std::string lastError() {
            std::string err = curl_easy_strerror(m_lastRes);
            if (m_errbuf[0]) {
                err += ": ";
                err += m_errbuf.data();
            }
            return err;
        }

        /**
         * Parse PEM encoded certificate.
         * @param data PEM encoded certificate
         * @return X509 structure, or NULL on error
         */
        static X509* parseCert(const char* cert_data, size_t len);

        future<void> startTask(IHttpTransfer* transfer);
        void completeTask(CURLcode resultCode);

    private:
        void init();

        /**
         * CURL response data callback
         * @param ptr Data to read
         * @param size Size of memory block
         * @param nmemb Number of memory blocks
         * @param client_ptr Pointer to HTTPClient
         * @return Number of copied bytes
         */
        static size_t write_data(void *ptr, size_t size, size_t nmemb, void *client_ptr);

        /**
         * CURL request data callback
         * @param ptr Data to read
         * @param size Size of memory block
         * @param nmemb Number of memory blocks
         * @param client_ptr Pointer to HTTPClient
         * @return Number of copied bytes
         */
        static size_t read_data(char* buffer, size_t size, size_t nitems, void* client_ptr);

        static int progress_callback(void *client_ptr, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

        /**
         * SSL context setup callback
         * @param curl CURL instance
         * @param ssl_ctx SSL_CTX
         * @param cert_data Pointer to PEM encoded certificate string data
         * @return CURLE_OK on success, some error otherwise
         */
        static CURLcode ssl_ctx_callback(CURL *curl, void *ssl_ctx, void *client_ptr);
    };

    inline bool operator==(HttpClient const& a, HttpClient const& b) {
        return a.getCURL() == b.getCURL();
    }
}



namespace std {
    // Make HttpClient hashable for HttpManager
    template<> struct hash<ceema::HttpClient>
    {
        typedef ceema::HttpClient argument_type;
        typedef std::size_t result_type;
        result_type operator()(argument_type const& s) const
        {
            return std::hash<const void*>{}(s.getCURL()) ;
        }
    };
}
