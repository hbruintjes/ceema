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

#include <curl/multi.h>
#include <unordered_set>
#include "HttpClient.h"

namespace ceema {

    /**
     * Manages various instances of HttpClient using curl_multi interface
     */
    class HttpManager {
    protected:
        CURLM* m_handle;
        std::string m_cert;

        std::unordered_set<HttpClient> m_clients;
        int m_running_handles;
    public:
        HttpManager();

        virtual ~HttpManager();

        virtual void* registerRead(int fd, void* ptr) = 0;
        virtual void* registerWrite(int fd, void* ptr) = 0;
        virtual void* unregister(int fd, void* ptr) = 0;

        virtual void registerTimeout(long timeout_ms) = 0;

        void set_cert(std::string cert) {
            m_cert = cert;
        }

        HttpClient& getFreeClient();

        void queueClient(HttpClient& client) {
            if (!client.getBusy()) {
                throw std::runtime_error("Attempt to queue taskless HTTP client");
            }
            curl_multi_add_handle(m_handle, client.getCURL());
        }

    protected:
        void checkMessages() {
            CURLMsg* m;
            do {
                int msgq = 0;
                m = curl_multi_info_read(m_handle, &msgq);
                if(m && (m->msg == CURLMSG_DONE)) {
                    curl_multi_remove_handle(m_handle, m->easy_handle);

                    HttpClient* client;
                    curl_easy_getinfo(m->easy_handle, CURLINFO_PRIVATE, &client);
                    client->completeTask(m->data.result);
                }
            } while(m);

        }

        static void timer_callback(CURLM *multi, long timeout_ms, void *userp) {
            ceema::HttpManager* mgr = static_cast<ceema::HttpManager*>(userp);
            mgr->registerTimeout(timeout_ms);
        }

        static int socket_callback(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp) {
            ceema::HttpManager* mgr = static_cast<ceema::HttpManager*>(userp);
            switch(action) {
                case CURL_POLL_IN:
                    socketp = mgr->registerRead(s, socketp);
                    break;
                case CURL_POLL_OUT:
                    socketp = mgr->registerWrite(s, socketp);
                    break;
                case CURL_POLL_INOUT:
                    socketp = mgr->registerRead(s, socketp);
                    socketp = mgr->registerWrite(s, socketp);
                    break;
                case CURL_POLL_REMOVE:
                    socketp = mgr->unregister(s, socketp);
                    break;
                case CURL_POLL_NONE:
                default:
                    break;
            }

            curl_multi_assign(mgr->m_handle, s, socketp);

            return CURLM_OK;
        }
    };

}
