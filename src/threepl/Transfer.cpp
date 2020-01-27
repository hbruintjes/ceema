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

#include "Transfer.h"
#include "ThreeplConnection.h"
#include "types/ptr_array.h"

#include <libpurple/debug.h>

#define G_STDIO_NO_WRAP_ON_UNIX 1
#include <glib/gstdio.h>
#include <fcntl.h>

#undef G_STDIO_NO_WRAP_ON_UNIX

PrplTransfer::PrplTransfer(ceema::BlobAPI& api, PurpleConnection* gc, const char *who, PurpleXferType xfer_type) :
    m_blobAPI(api)
{
    m_xfer = purple_xfer_new(purple_connection_get_account(gc), xfer_type, who);
    m_xfer->data = this;
    purple_xfer_ref(m_xfer);

    purple_xfer_set_init_fnc(m_xfer, &PrplTransfer::xfer_init_cb);

    purple_xfer_set_start_fnc(m_xfer, &PrplTransfer::xfer_start_cb);
    purple_xfer_set_end_fnc(m_xfer, &PrplTransfer::xfer_end_cb);

    purple_xfer_set_cancel_send_fnc(m_xfer, &PrplTransfer::xfer_cancel_cb);
    purple_xfer_set_cancel_recv_fnc(m_xfer, &PrplTransfer::xfer_cancel_cb);
    purple_xfer_set_request_denied_fnc(m_xfer, &PrplTransfer::xfer_request_denied_cb);

    purple_xfer_set_read_fnc(m_xfer, &PrplTransfer::xfer_read_cb);
    purple_xfer_set_write_fnc(m_xfer, &PrplTransfer::xfer_write_cb);
    purple_xfer_set_ack_fnc(m_xfer, &PrplTransfer::xfer_ack_cb);

    // Take over file handling for encryption/decryption
    m_ops = *purple_xfer_get_ui_ops(m_xfer);
    m_ops.ui_read = &PrplTransfer::ui_read;
    m_ops.ui_write = &PrplTransfer::ui_write;
    m_ops.data_not_sent = &PrplTransfer::data_not_sent;
    m_xfer->ui_ops = &m_ops;

    purple_xfer_ui_ready(m_xfer);
}

PrplTransfer::~PrplTransfer() {
    purple_xfer_unref(m_xfer);
}

gssize PrplTransfer::on_ui_write(const guchar *buffer, gssize size) {
    m_data.insert(m_data.end(), buffer, buffer+size);

    purple_xfer_ui_ready(m_xfer);

    return size;
}

gssize PrplTransfer::on_ui_read(guchar **buffer, gssize size) {
    gssize avail = std::min(size, (gssize)m_data.size());
    *buffer = static_cast<guchar*>(g_malloc(avail));
    std::copy(m_data.begin(), m_data.begin()+avail, *buffer);
    m_data.erase(m_data.begin(), m_data.begin()+avail);

    purple_xfer_ui_ready(m_xfer);

    return avail;
}

void PrplTransfer::on_data_not_sent(const guchar *buffer, gsize size) {
    m_data.insert(m_data.begin(), buffer, buffer+size);
}

PrplUploadTransfer::PrplUploadTransfer(ceema::BlobAPI& api, PurpleConnection* gc, const char *who) :
        PrplTransfer(api, gc, who, PURPLE_XFER_SEND) {
}

PrplUploadTransfer::~PrplUploadTransfer() {
}

ssize_t PrplUploadTransfer::read(unsigned char* buffer, std::size_t expected) {
    // CURL requests data, indicate purple transfer is ready to fill the buffer
    if (m_buffer.empty() && purple_xfer_get_status(xfer()) == PURPLE_XFER_STATUS_STARTED) {
        purple_xfer_prpl_ready(xfer());
    }
    if (m_buffer.empty()) {
        // EOF
        return 0;
    }

    auto copy_size = std::min(expected, m_buffer.size());
    std::copy(m_buffer.begin(), m_buffer.begin()+copy_size, buffer);
    m_buffer.erase(m_buffer.begin(), m_buffer.begin()+copy_size);

    return copy_size;
}

void PrplUploadTransfer::on_xfer_init() {
    // When sending file, create a temporary copy, encrypt it (using mapped files)
    // and point the local filename to it instead
    GError* err = nullptr;

    const char* filename = purple_xfer_get_local_filename(xfer());

    m_blobData.filename = purple_xfer_get_filename(xfer());
    m_blobData.localFilename = filename;

    gint localfile = g_open(filename, O_RDONLY, 0);

    GMappedFile * mapped_localfile = g_mapped_file_new_from_fd(localfile, FALSE, &err);
    const unsigned char* localdata = reinterpret_cast<const unsigned char*>(g_mapped_file_get_contents(mapped_localfile));
    auto localdata_array = ceema::ptr_array<const unsigned char>(localdata, g_mapped_file_get_length(mapped_localfile));

    //TODO: could encrypt into new memory buffer, or reuse the memory mapped file?
    m_data.insert(m_data.begin(), localdata_array.begin(), localdata_array.end());

    bool ok = this->encrypt();
    LOG_DBG("Encrypt: " << ok);

    purple_xfer_set_size(xfer(), m_data.size());

    g_mapped_file_unref(mapped_localfile);

    g_close(localfile, &err);

    purple_xfer_prepare_thumbnail(xfer(), "jpeg");

    gconstpointer thumb;
    gsize thumb_size;
    if ((thumb = purple_xfer_get_thumbnail(xfer(), &thumb_size))) {
        ceema::byte_vector vec;
        vec.insert(vec.begin(), static_cast<unsigned char const *>(thumb), static_cast<unsigned char const *>(thumb) + thumb_size);
        get_thumb(std::move(vec)).next([this](ceema::future<ceema::blob_id> fut) {
            m_blobData.hasThumb = false;
            try {
                m_blobData.thumb = fut.get();
                m_blobData.hasThumb = true;
            } catch(std::exception& e) {
                LOG_DBG("Thumbnail upload exception: " << e.what());
                // Continue, no thumbnail is not critical
            }

            purple_xfer_start(xfer(), -1, NULL, 0);
        });
    } else {
        m_blobData.hasThumb = false;
        // Initiate the transfer immediately
        purple_xfer_start(xfer(), -1, NULL, 0);
    }
}

void PrplUploadTransfer::on_xfer_start() {
    api().upload(this);
}

gssize PrplUploadTransfer::on_xfer_write(const guchar * buffer, size_t size) {
    m_buffer.insert(m_buffer.end(), buffer, buffer + size);
    return size;
}

PrplDownloadTransfer::PrplDownloadTransfer(ceema::BlobAPI& api, ceema::blob_id id, ceema::blob_size size,
                                           PurpleConnection* gc, const char *who) :
        PrplTransfer(api, gc, who, PURPLE_XFER_RECEIVE), m_id(id) {
    purple_xfer_set_size(xfer(), size);
}

PrplDownloadTransfer::~PrplDownloadTransfer() {
}

ssize_t PrplDownloadTransfer::write(const unsigned char* buffer, std::size_t available) {
    if (purple_xfer_get_status(xfer()) != PURPLE_XFER_STATUS_STARTED) {
        // error
        return -1;
    }

    m_buffer.insert(m_buffer.end(), buffer, buffer + available);

    // Buffer has data, inform purple
    purple_xfer_prpl_ready(xfer());

    return available;
}

void PrplDownloadTransfer::on_xfer_start() {
    purple_debug_info("threepl", "start download\n");
    api().downloadFile(this, m_id);
}

gssize PrplDownloadTransfer::on_xfer_read(guchar ** buffer) {
    size_t size = m_buffer.size();
    purple_debug_info("threepl", "on_xfer_read %lu\n", size);

    *buffer = static_cast<guchar*>(g_malloc(size));
    std::copy(m_buffer.begin(), m_buffer.end(), *buffer);
    m_buffer.clear();

    return size;
}

void PrplDownloadTransfer::on_xfer_end() {
    // When receiving a file, upon completion decrypt it

    LOG_DBG("Open downloaded file: ");
    FILE* localfile = g_fopen(purple_xfer_get_local_filename(xfer()), "wb");
    LOG_DBG("Open downloaded file: " << (localfile != NULL));

    bool ok = decrypt();
    LOG_DBG("Decrypt: " << ok);

    ::fwrite(m_data.data(), 1, m_data.size(), localfile);
    ::fclose(localfile);

    PrplTransfer::on_xfer_end();
}

void PrplTransfer::xfer_request_denied_cb(PurpleXfer *xfer) {
    purple_debug_info("threepl", "xfer_request_denied_cb\n");
    // User decided to deny the incoming file
    // Cancel and cleanup transfer data
    // xfer will unref itself
    static_cast<PrplTransfer*>(xfer->data)->on_xfer_request_denied();
}

void PrplTransfer::xfer_init_cb(PurpleXfer *xfer) {
    purple_debug_info("threepl", "threepl_transfer_init_cb\n");
    // Prepare the transfer to it can be presented to the user
    // Must be implemented
    static_cast<PrplTransfer*>(xfer->data)->on_xfer_init();
}

void PrplTransfer::xfer_cancel_cb(PurpleXfer *xfer) {
    purple_debug_info("threepl", "xfer_cancel_cb\n");
    // User decided to cancel the transfer
    // Do cleanup of transfer
    // xfer will unref itself
    static_cast<PrplTransfer*>(xfer->data)->on_xfer_cancel();
}

void PrplTransfer::xfer_start_cb(PurpleXfer *xfer) {
    purple_debug_info("threepl", "xfer_start_cb\n");
    // xfer has been stated, FP's have been initialized etc. Waiting for data
    static_cast<PrplTransfer*>(xfer->data)->on_xfer_start();
}

void PrplTransfer::xfer_end_cb(PurpleXfer *xfer) {
    purple_debug_info("threepl", "xfer_end_cb\n");
    // called on normal ending (purple_xfer_end), when all data has been transmitted
    // Cleanup transfer
    // xfer will unref itself
    static_cast<PrplTransfer*>(xfer->data)->on_xfer_end();
}

void PrplTransfer::xfer_ack_cb(PurpleXfer *xfer, const guchar * data, size_t size) {
    purple_debug_info("threepl", "xfer_ack_cb\n");
    static_cast<PrplTransfer*>(xfer->data)->on_xfer_ack(data, size);
}

gssize PrplTransfer::xfer_read_cb(guchar ** data, PurpleXfer *xfer) {
    purple_debug_info("threepl", "xfer_read_cb\n");
    return static_cast<PrplTransfer*>(xfer->data)->on_xfer_read(data);

    size_t all_read = purple_xfer_get_bytes_sent (xfer);
    purple_debug_info("threepl", "xfer_read_overall: %lu\n", all_read);
}

gssize PrplTransfer::xfer_write_cb(const guchar * data, size_t size, PurpleXfer *xfer) {
    purple_debug_error("threepl", "xfer_write_cb %d\n", (int)size);
    return static_cast<PrplTransfer*>(xfer->data)->on_xfer_write(data, size);
}

gssize PrplTransfer::ui_write(PurpleXfer *xfer, const guchar *buffer, gssize size) {
    purple_debug_error("threepl", "ui_write %d\n", (int)size);
    return static_cast<PrplTransfer*>(xfer->data)->on_ui_write(buffer, size);
}

gssize PrplTransfer::ui_read(PurpleXfer *xfer, guchar **buffer, gssize size) {
    purple_debug_error("threepl", "ui_read %d\n", (int)size);
    return static_cast<PrplTransfer*>(xfer->data)->on_ui_read(buffer, size);
}

void PrplTransfer::data_not_sent(PurpleXfer *xfer, const guchar *buffer, gsize size) {
    purple_debug_error("threepl", "data_not_sent %d\n", (int)size);
    static_cast<PrplTransfer*>(xfer->data)->on_data_not_sent(buffer, size);
}