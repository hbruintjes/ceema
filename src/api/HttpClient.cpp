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

#include "HttpClient.h"

#include "HttpManager.h"


#include <openssl/ossl_typ.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <curl/multi.h>

namespace ceema {

    HttpClient::~HttpClient() {
        close();
        if (m_cert) {
            X509_free(m_cert);
        }
    }

    future<byte_vector> HttpClient::get(std::string const& url) {
        get(url, &m_bufferedTransfer);
        return m_bufferedTransfer.get_future();
    }

    future<void> HttpClient::get(std::string const& url, IHttpTransfer* transfer) {
        CURLcode res;
        if ((res = curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str())) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        if ((res = curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1L)) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        if ((res = curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, NULL)) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        return startTask(transfer);
    }

    future<byte_vector> HttpClient::post(std::string url, byte_vector data) {
        m_bufferedTransfer.set_buffer(std::move(data));
        post(url, &m_bufferedTransfer);
        return m_bufferedTransfer.get_future();
    }

    void HttpClient::post(std::string url, IHttpTransfer* transfer) {
        CURLcode res;
        if ((res = curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str())) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        if ((res = curl_easy_setopt(m_curl, CURLOPT_POST, 1L)) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        curl_slist* headers = curl_slist_append(NULL, "Transfer-Encoding: chunked");
        if ((res = curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers)) != CURLE_OK) {
            curl_slist_free_all(headers);
            throw std::runtime_error(curl_easy_strerror(res));
        }

        startTask(transfer).next([headers](future<void> fut) {
            curl_slist_free_all(headers);
            fut.get();
        });
    }

    future<byte_vector> HttpClient::postFile(std::string url, byte_vector data, std::string const& filename) {
        m_bufferedTransfer.set_buffer(std::move(data));
        postFile(url, &m_bufferedTransfer, filename);
        return m_bufferedTransfer.get_future();
    }

    void HttpClient::postFile(std::string url, IHttpTransfer* transfer, std::string const& filename) {
        if (transfer->size() == -1) {
            throw std::runtime_error("Invalid file size to POST");
        }

        CURLcode res;
        if ((res = curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str())) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        if ((res = curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, NULL)) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        struct curl_httppost* post = NULL;
        struct curl_httppost* last = NULL;
        CURLFORMcode form_res;
        if ((form_res = curl_formadd(&post, &last, CURLFORM_COPYNAME, filename.c_str(),
                                     CURLFORM_STREAM, this,
                                     CURLFORM_CONTENTSLENGTH, static_cast<long>(transfer->size()),
                                     CURLFORM_FILENAME, filename.c_str(),
                                     CURLFORM_END)) != 0) {
            throw std::runtime_error("curl_formadd failed: " + std::to_string(form_res));
        }

        if ((res = curl_easy_setopt(m_curl, CURLOPT_HTTPPOST, post)) != CURLE_OK) {
            curl_formfree(post);
            throw std::runtime_error(curl_easy_strerror(res));
        }

        startTask(transfer).next([post](future<void> fut) {
            curl_formfree(post);
            fut.get();
        });
    }

    future<void> HttpClient::startTask(IHttpTransfer* transfer) {
        if (m_busy) {
            throw std::runtime_error("Attempt to use busy HttpClient");
        }

        m_errbuf[0] = 0;
        m_busy = true;
        m_transfer = transfer;

        m_currentTask = promise<void>{};
        m_manager.queueClient(*this);

        if (m_transfer) {
            m_transfer->onStart();
        }

        return m_currentTask.get_future();
    }

    void HttpClient::completeTask(CURLcode resultCode) {
        if (resultCode == CURLE_OK) {
            if (m_transfer) {
                m_transfer->onComplete();
            }
            m_currentTask.set_value();
        } else {
            if (m_transfer) {
                m_transfer->onFailed(resultCode, m_errbuf.data());
            }
            m_currentTask.set_exception(std::make_exception_ptr(std::runtime_error(lastError())));
        }

        m_transfer = nullptr;

        m_busy = false;
    }

    void HttpClient::set_cert(X509* cert) {
        if (m_cert) {
            X509_free(m_cert);
        }
        m_cert = X509_dup(cert);
    }

    X509* HttpClient::parseCert(const char* cert_data, size_t len) {
        BIO* bio = BIO_new_mem_buf(cert_data, len);
        if (bio == NULL) {
            throw std::runtime_error("Unable to allocate BIO");
        }
        X509* cert = PEM_read_bio_X509(bio, NULL, 0, NULL);
        BIO_free(bio);
        return cert;
    }

    void HttpClient::init() {
        if (m_curl) {
            return;
        }

        m_curl = curl_easy_init();
        if (!m_curl) {
            throw std::runtime_error("Unable to init CURL");
        }

        curl_easy_setopt(m_curl, CURLOPT_PRIVATE, this);

        CURLcode res;
        if ((res = curl_easy_setopt(m_curl, CURLOPT_USERAGENT, m_userAgent.c_str())) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        if ((res = curl_easy_setopt(m_curl, CURLOPT_ACCEPT_ENCODING, "")) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        if ((res = curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &HttpClient::write_data)) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }
        if ((res = curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this)) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        if ((res = curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, &HttpClient::read_data)) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }
        if ((res = curl_easy_setopt(m_curl, CURLOPT_READDATA, this)) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        if ((res = curl_easy_setopt(m_curl, CURLOPT_XFERINFOFUNCTION, &HttpClient::progress_callback)) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }
        if ((res = curl_easy_setopt(m_curl, CURLOPT_XFERINFODATA, this)) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }
        if ((res = curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L)) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        if ((res = curl_easy_setopt(m_curl, CURLOPT_SSL_CTX_FUNCTION, &HttpClient::ssl_ctx_callback)) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        if ((res = curl_easy_setopt(m_curl, CURLOPT_SSL_CTX_DATA, this)) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        if ((res = curl_easy_setopt(m_curl, CURLOPT_FAILONERROR, 1L)) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        if ((res = curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, m_errbuf.data())) != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        //curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
    }

    size_t HttpClient::write_data(void *ptr, size_t size, size_t nmemb, void *client_ptr) {
        HttpClient& client = *static_cast<HttpClient*>(client_ptr);

        // If writing to buffer, simply insert the data
        unsigned char* ptr_data = static_cast<unsigned char*>(ptr);

        ssize_t res = client.m_transfer->write(ptr_data, size*nmemb);
        if (res < 0) {
            return 0;
        }
        return static_cast<std::size_t>(res);
    }

    size_t HttpClient::read_data(char* buffer, size_t size, size_t nitems, void* client_ptr) {
        HttpClient& client = *static_cast<HttpClient*>(client_ptr);

        unsigned char* buffer_data = reinterpret_cast<unsigned char*>(buffer);
        ssize_t res = client.m_transfer->read(buffer_data, size*nitems);
        if (res < 0) {
            return 0;
        }
        return static_cast<std::size_t>(res);
    }


    int HttpClient::progress_callback(void *client_ptr, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
        HttpClient& client = *static_cast<HttpClient*>(client_ptr);

        //LOG_DBG("progress_callback " << dltotal << " " << dlnow << " "  << ultotal << " "  << ulnow);

        if (client.m_transfer) {
            return client.m_transfer->cancelled() ? 1 : 0;
        } else {
            return 0;
        }
    }

    CURLcode HttpClient::ssl_ctx_callback(CURL *curl, void *ssl_ctx, void *client_ptr) {
        HttpClient& client = *static_cast<HttpClient*>(client_ptr);

        LOG_DEBUG(logging::loggerRoot, "Inserting SSL cert");
        CURLcode status = CURLE_OK;

        X509_STORE* store = SSL_CTX_get_cert_store(static_cast<SSL_CTX*>(ssl_ctx));
        if (!X509_STORE_add_cert(store, client.m_cert)) {
            // Error adding cert
            status = CURLE_SSL_CERTPROBLEM;
        }

        return status;
    }

}