#include "HttpClient.h"

#include <mbedtls/ssl.h>
#include <mbedtls/x509.h>

namespace ceema {
    CURLcode HttpClient::ssl_ctx_callback_mbedtls(CURL *curl, void *ssl_ctx, void *client_ptr) {
        HttpClient& client = *static_cast<HttpClient*>(client_ptr);

        if (client.m_cert.empty()) {
            return CURLE_OK;
        }

        LOG_DEBUG(logging::loggerRoot, "Inserting SSL cert");
        CURLcode status = CURLE_OK;

        auto mbed_ctx = static_cast<mbedtls_ssl_config*>(ssl_ctx);

        mbedtls_x509_crt cacert;
        mbedtls_x509_crt_init( &cacert );

        if (mbedtls_x509_crt_parse(
                &cacert,
                reinterpret_cast<const unsigned char*>(client.m_cert.c_str()),
                client.m_cert.size() + 1) != 0) {
            status = CURLE_SSL_CERTPROBLEM;
        } else {
            mbedtls_ssl_conf_ca_chain(mbed_ctx, &cacert, nullptr);
        }

        return status;
    }
}