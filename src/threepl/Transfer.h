//
// Created by hbruintjes on 21/03/17.
//

#pragma once

#include <libpurple/ft.h>

#include <api/HttpClient.h>
#include <api/BlobAPI.h>
#include <api/BlobTransfer.h>

class PrplTransfer {
    PurpleXfer* m_xfer;
    ceema::BlobAPI& m_blobAPI;
    PurpleXferUiOps m_ops;

protected:
    ceema::byte_vector m_data;

public:
    PrplTransfer(ceema::BlobAPI& api, PurpleConnection* gc, const char *who, PurpleXferType xfer_type);

    virtual ~PrplTransfer() = 0;

    PurpleXfer* xfer() const {
        return m_xfer;
    }

    ceema::BlobAPI& api() {
        return m_blobAPI;
    }

protected:
    virtual void on_xfer_request_denied() {
        on_xfer_done();
    }

    virtual void on_xfer_init() {
        purple_xfer_start(xfer(), -1, NULL, 0);
    }

    virtual void on_xfer_cancel() {
        on_xfer_done();
    }

    virtual void on_xfer_start() {}

    virtual void on_xfer_end() {
        on_xfer_done();
    }

    virtual void on_xfer_ack(const guchar *, size_t) {}
    virtual gssize on_xfer_read(guchar **) {
        return -1;
    }
    virtual gssize on_xfer_write(const guchar *, size_t) {
        return -1;
    }

    virtual void on_xfer_done() {
        m_data.clear();
    }

    virtual gssize on_ui_write(const guchar *buffer, gssize size);
    virtual gssize on_ui_read(guchar **buffer, gssize size);
    virtual void on_data_not_sent(const guchar *buffer, gsize size);

private:
    static void xfer_request_denied_cb(PurpleXfer *xfer);

    static void xfer_init_cb(PurpleXfer *xfer);

    static void xfer_cancel_cb(PurpleXfer *xfer);

    static void xfer_start_cb(PurpleXfer *xfer);

    static void xfer_end_cb(PurpleXfer *xfer);

    static void xfer_ack_cb(PurpleXfer *xfer, const guchar * data, size_t size);

    static gssize xfer_read_cb(guchar ** data, PurpleXfer *xfer);

    static gssize xfer_write_cb(const guchar * data, size_t size, PurpleXfer *xfer);

    static gssize ui_write(PurpleXfer *xfer, const guchar *buffer, gssize size);

    static gssize ui_read(PurpleXfer *xfer, guchar **buffer, gssize size);

    static void data_not_sent(PurpleXfer *xfer, const guchar *buffer, gsize size);
};

struct UploadData {
    bool isLegacy;

    union {
        ceema::Blob blob;
        ceema::LegacyBlob legacyBlob;
    };

    bool hasThumb;
    ceema::blob_id thumb;

    std::string filename;
    std::string localFilename;
};

class PrplUploadTransfer: public PrplTransfer, public ceema::FutureHttpTransfer<UploadData> {
    ceema::byte_vector m_buffer;
    ceema::byte_vector m_idbuffer;

protected:
    UploadData m_blobData;

public:
    PrplUploadTransfer(ceema::BlobAPI& api, PurpleConnection* gc, const char *who);

    ~PrplUploadTransfer();

    ssize_t read(unsigned char* buffer, std::size_t expected) override;

    ssize_t write(unsigned char const* buffer, std::size_t available) override {
        m_idbuffer.insert(m_idbuffer.end(), buffer, buffer + available);
        return available;
    }

    void onFailed(CURLcode errCode, const char* errMsg) override {
        if (xfer()) {
            purple_xfer_cancel_remote(xfer());
        }
        ceema::FutureHttpTransfer<UploadData>::onFailed(errCode, errMsg);
    }

    bool cancelled() override {
        return (purple_xfer_get_status(xfer()) == PURPLE_XFER_STATUS_CANCEL_LOCAL) ||
               (purple_xfer_get_status(xfer()) == PURPLE_XFER_STATUS_CANCEL_REMOTE);
    }

    ssize_t size() override {
        return purple_xfer_get_size(xfer());
    }

protected:
    void on_xfer_init() override;
    void on_xfer_start() override;

    gssize on_xfer_write(const guchar *, size_t) override;

    UploadData&& get_value() override {
        //TODO: This is specific to blob, not legacy
        ceema::hex_decode(m_idbuffer, m_blobData.blob.id);
        return std::move(m_blobData);
    }

    virtual ceema::future<ceema::blob_id> get_thumb(ceema::byte_vector data) = 0;

    virtual bool encrypt() = 0;
};

class PrplBlobUploadTransfer: public PrplUploadTransfer {
    ceema::BlobType m_type;

public:
    PrplBlobUploadTransfer(ceema::BlobAPI& api, ceema::BlobType type, PurpleConnection* gc, const char *who) :
            PrplUploadTransfer(api, gc, who), m_type(type) {
        m_blobData.blob.key = ceema::crypto::generate_shared_key();
    }

protected:
    ceema::future<ceema::blob_id> get_thumb(ceema::byte_vector data) override {
        //TODO: selecting type like this isn't pretty

        auto transfer = new ceema::BlobUploadTransfer(
                std::move(data),
                m_type == ceema::BlobType::FILE? ceema::BlobType::FILE_THUMB : ceema::BlobType::VIDEO_THUMB,
                m_blobData.blob.key);
        api().upload(transfer);
        return transfer->get_future().next([](ceema::future<ceema::Blob> fut) {
            auto blob = fut.get();
            return blob.id;
        });
    }

    bool encrypt() override {
        m_blobData.blob.size = m_data.size();
        LOG_DBG("Encrypt blob using " << m_blobData.blob.key);
        return ceema::BlobAPI::encrypt(m_data, m_type, m_blobData.blob.key);
    }
};

class PrplLegacyBlobUploadTransfer: public PrplUploadTransfer {
    ceema::public_key m_pk;
    ceema::private_key m_sk;

public:
    PrplLegacyBlobUploadTransfer(ceema::BlobAPI& api, ceema::public_key pk, ceema::private_key sk,
                           PurpleConnection* gc, const char *who) :
            PrplUploadTransfer(api, gc, who), m_pk(pk), m_sk(sk) {
        m_blobData.legacyBlob.n = ceema::crypto::generate_nonce();
    }

protected:
    ceema::future<ceema::blob_id> get_thumb(ceema::byte_vector data) override {
        auto transfer = new ceema::LegacyBlobUploadTransfer(std::move(data), m_pk, m_sk, m_blobData.legacyBlob.n);
        api().upload(transfer);
        return transfer->get_future().next([](ceema::future<ceema::LegacyBlob> fut) {
            auto blob = fut.get();
            return blob.id;
        });
    }

    bool encrypt() override {
        m_blobData.legacyBlob.size = m_data.size();
        return ceema::BlobAPI::encrypt(m_data, m_blobData.legacyBlob.n, m_pk, m_sk);
    }

};

class PrplDownloadTransfer: public PrplTransfer, public ceema::FutureHttpTransfer<void> {
    // Buffer to sync between CURL and purple
    ceema::byte_vector m_buffer;
    ceema::blob_id m_id;

public:
    PrplDownloadTransfer(ceema::BlobAPI& api, ceema::blob_id id, ceema::blob_size size,
                         PurpleConnection* gc, const char *who);

    ~PrplDownloadTransfer();

    ssize_t write(const unsigned char* buffer, std::size_t available) override;

    void onFailed(CURLcode errCode, const char* errMsg) override {
        if (xfer()) {
            purple_xfer_cancel_remote(xfer());
        }
        ceema::FutureHttpTransfer<void>::onFailed(errCode, errMsg);
    }

    bool cancelled() override {
        return (purple_xfer_get_status(xfer()) == PURPLE_XFER_STATUS_CANCEL_LOCAL) ||
               (purple_xfer_get_status(xfer()) == PURPLE_XFER_STATUS_CANCEL_REMOTE);
    }

    ssize_t size() override {
        return purple_xfer_get_size(xfer());
    }

protected:
    void on_xfer_start() override;
    gssize on_xfer_read(guchar **) override;
    void on_xfer_end() override;

    virtual bool decrypt() = 0;
};

class PrplBlobDownloadTransfer : public PrplDownloadTransfer{
    ceema::Blob m_blob;
    ceema::BlobType m_type;

public:
    PrplBlobDownloadTransfer(ceema::BlobAPI& api, ceema::Blob blob, ceema::BlobType type,
    PurpleConnection* gc, const char *who) :
            PrplDownloadTransfer(api, blob.id, blob.size + (type == ceema::BlobType::ICON?0:crypto_box_MACBYTES), gc, who),
            m_blob(blob), m_type(type) {}

protected:
    bool decrypt() override {
        LOG_DBG("Decrypt blob using " << m_blob.key);
        return ceema::BlobAPI::decrypt(m_data, m_type, m_blob.key);
    }
};

class PrplLegacyBlobDownloadTransfer : public PrplDownloadTransfer {
    ceema::LegacyBlob m_blob;
    ceema::public_key m_pk;
    ceema::private_key m_sk;

public:
    PrplLegacyBlobDownloadTransfer(ceema::BlobAPI& api, ceema::LegacyBlob blob, ceema::public_key pk,
                                   ceema::private_key sk, PurpleConnection* gc, const char *who) :
            PrplDownloadTransfer(api, blob.id, blob.size, gc, who), m_blob(blob), m_pk(pk), m_sk(sk) {}

protected:
    bool decrypt() override {
        return ceema::BlobAPI::decrypt(m_data, m_blob.n, m_pk, m_sk);
    }
};