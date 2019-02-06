#include "HttpClient.h"

#include <openssl/ossl_typ.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

namespace ceema {
    CURLcode HttpClient::ssl_ctx_callback_openssl(CURL *curl, void *ssl_ctx, void *client_ptr) {
        HttpClient& client = *static_cast<HttpClient*>(client_ptr);

        if (client.m_cert.empty()) {
            return CURLE_OK;
        }

        LOG_DEBUG(logging::loggerRoot, "Inserting SSL cert");
        CURLcode status = CURLE_OK;

        auto bio = BIO_new_mem_buf(client.m_cert.c_str(), client.m_cert.size());
        if (bio == NULL) {
            return CURLE_OUT_OF_MEMORY;
        }
        auto cert = PEM_read_bio_X509(bio, NULL, 0, NULL);
        BIO_free(bio);
        if (cert == NULL) {
            return CURLE_SSL_CERTPROBLEM;
        }

        auto store = SSL_CTX_get_cert_store(static_cast<SSL_CTX*>(ssl_ctx));
        if (!X509_STORE_add_cert(store, cert)) {
            // Error adding cert
            status = CURLE_SSL_CERTPROBLEM;
        }
        X509_free(cert);

        return status;
    }
}
