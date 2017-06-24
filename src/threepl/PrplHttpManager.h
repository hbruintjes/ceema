//
// Created by hbruintjes on 24/02/17.
//

#ifndef CEEMA_PRPLHTTPMANAGER_H
#define CEEMA_PRPLHTTPMANAGER_H

#include <api/HttpManager.h>
#include <libpurple/eventloop.h>
#include <curl/curl.h>

class PrplHttpManager : public ceema::HttpManager {
    struct SocketCallbacks {
        guint read;
        guint write;
    };

    guint timeout;

public:
    void* registerRead(int fd, void* ptr) override {
        SocketCallbacks* callbacks = static_cast<SocketCallbacks*>(ptr);
        if (!callbacks) {
            callbacks = new SocketCallbacks{};
        }
        if (!callbacks->read) {
            callbacks->read = purple_input_add(fd, static_cast<PurpleInputCondition>(PURPLE_INPUT_READ),
                                               &on_http_data_event, this);
        }
        return callbacks;
    }

    void* registerWrite(int fd, void* ptr) override {
        SocketCallbacks* callbacks = static_cast<SocketCallbacks*>(ptr);
        if (!callbacks) {
            callbacks = new SocketCallbacks{};
        }
        if (!callbacks->write) {
            callbacks->write = purple_input_add(fd, static_cast<PurpleInputCondition>(PURPLE_INPUT_WRITE),
                                                &on_http_data_event, this);
        }
        return callbacks;
    }

    void* unregister(int fd, void* ptr) override {
        if (ptr) {
            SocketCallbacks* callbacks = static_cast<SocketCallbacks*>(ptr);

            if (callbacks->read) {
                purple_input_remove(callbacks->read);
            }
            if (callbacks->write) {
                purple_input_remove(callbacks->write);
            }

            delete callbacks;
        }
        return nullptr;
    }

    void registerTimeout(long timeout_ms) override {
        if (timeout_ms == -1) {
            purple_timeout_remove(timeout);
        } else {
            timeout = purple_timeout_add(timeout_ms, &on_http_timeout_event, this);
        }
    }

protected:
    static void on_http_data_event(gpointer data, gint fd, PurpleInputCondition condition) {
        PrplHttpManager* mgr = static_cast<PrplHttpManager*>(data);

        int running_handles = 0;
        int ev_bitmask = (condition & PURPLE_INPUT_READ) ? CURL_CSELECT_IN : 0;
        ev_bitmask |= (condition & PURPLE_INPUT_WRITE) ? CURL_CSELECT_OUT : 0;

        curl_multi_socket_action(mgr->m_handle, fd, ev_bitmask, &running_handles);

        mgr->checkMessages();
    }

    static gboolean on_http_timeout_event(gpointer data) {
        PrplHttpManager* mgr = static_cast<PrplHttpManager*>(data);

        int running_handles = 0;
        curl_multi_socket_action(mgr->m_handle, CURL_SOCKET_TIMEOUT, 0, &running_handles);

        mgr->checkMessages();

        return FALSE; // curl asks for non-repeating
    }

};


#endif //CEEMA_PRPLHTTPMANAGER_H
