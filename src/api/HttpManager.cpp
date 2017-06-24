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

#include "HttpManager.h"
#include <openssl/x509.h>

namespace ceema {
    HttpManager::HttpManager() : m_cert(NULL), m_running_handles(0) {
        m_handle = curl_multi_init();

        curl_multi_setopt(m_handle, CURLMOPT_SOCKETFUNCTION, &socket_callback);
        curl_multi_setopt(m_handle, CURLMOPT_SOCKETDATA, this);
        curl_multi_setopt(m_handle, CURLMOPT_TIMERFUNCTION, &timer_callback);
        curl_multi_setopt(m_handle, CURLMOPT_TIMERDATA, this);

    }

    HttpManager::~HttpManager() {
        for(auto& client: m_clients) {
            curl_multi_remove_handle(m_handle, client.getCURL());
        }
        m_clients.clear();

        curl_multi_cleanup(m_handle);

        if (m_cert) {
            X509_free(m_cert);
        }
    }

    void HttpManager::set_cert(X509* cert) {
        if (m_cert) {
            X509_free(m_cert);
        }
        m_cert = X509_dup(cert);
    }

    HttpClient& HttpManager::getFreeClient() {
        //TODO: invalid const-correctness
        for(auto& client: m_clients) {
            if (!client.getBusy()) {
                return const_cast<HttpClient&>(client);
            }
        }
        auto emplace_res = m_clients.emplace(*this, std::string{"Threema/Ceema"});
        auto& client = const_cast<HttpClient&>(*emplace_res.first);
        client.set_cert(m_cert);
        return client;
    }
}