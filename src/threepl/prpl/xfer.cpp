//
// Created by harold on 30/05/17.
//

#include "xfer.h"

#include <threepl/ThreeplConnection.h>

#include <gio/gio.h>

gboolean threepl_can_receive_file(PurpleConnection *, const char *who) {
    // Conditional based on existence of PK? Only relevant for images,
    // otherwise just send a message and hope for the best
    return TRUE;
}

void threepl_send_file(PurpleConnection* gc, const char *who, const char *filename) {
    ThreeplConnection* connection = static_cast<ThreeplConnection*>(purple_connection_get_protocol_data(gc));
    PrplBlobUploadTransfer* transfer = connection->new_xfer(gc, who);

    auto cid = ceema::client_id::fromString(who);

    transfer->get_future().next([connection, cid](ceema::future<UploadData> fut){
        LOG_DBG("Sending transfer message");
        UploadData data = fut.get();
        //TODO: catch exception and write message

        //TODO: send message with blob

        ceema::PayloadFile payload;

        payload.id = data.blob.id;
        payload.key = data.blob.key;
        payload.size = data.blob.size;

        payload.has_thumb = data.hasThumb;
        payload.thumb_id = data.thumb;
        payload.filename = data.filename;
        payload.flag = ceema::PayloadFile::FileFlag::DEFAULT;

        // Attempt to figure out the mimetype
        GError* err;
        struct _GFile * gfile = g_file_new_for_path(data.localFilename.c_str());
        struct _GFileInfo * gfileinfo = g_file_query_info (gfile, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                                           G_FILE_QUERY_INFO_NONE, NULL, &err);

        payload.mime_type = g_file_info_get_content_type(gfileinfo);

        g_object_unref(gfileinfo);
        g_object_unref(gfile);

        connection->send_message(cid, payload);
    });

    auto xfer = transfer->xfer();
    if (filename) {
        // Send via via DnD or similar, filename is known
        purple_xfer_request_accepted(xfer, filename);
    } else {
        // Send via menu, request name via dialog
        purple_xfer_request(xfer);
    }
}