#include "HttpClient.h"

#include <wolfssl/ssl.h>

namespace ceema {
    CURLcode HttpClient::ssl_ctx_callback_wolfssl(CURL *curl, void *ssl_ctx, void *client_ptr) {
        HttpClient& client = *static_cast<HttpClient*>(client_ptr);

        if (client.m_cert.empty()) {
            return CURLE_OK;
        }

        LOG_DEBUG(logging::loggerRoot, "Inserting SSL cert");
        CURLcode status = CURLE_OK;

        auto res = wolfSSL_CTX_use_certificate_buffer(
                static_cast<WOLFSSL_CTX*>(ssl_ctx),
                reinterpret_cast<const unsigned char*>(client.m_cert.c_str()), client.m_cert.size(),
                SSL_FILETYPE_PEM);
        if (res != SSL_SUCCESS) {
            // Error adding cert
            status = CURLE_SSL_CERTPROBLEM;
        }

        return status;
    }

}
