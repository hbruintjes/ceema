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

#include <encoding/crypto.h>
#include <encoding/hex.h>
#include "BlobAPI.h"

namespace ceema {
    BlobAPI::BlobAPI(HttpManager& manager, bool useTLS) : API(manager), m_useTLS(useTLS) {
        if (m_useTLS) {
            m_url = "https://upload.blob.threema.ch/upload";
        } else {
            m_url = "http://blob-upload.threema.ch/upload";
        }
    }

    void BlobAPI::upload(IHttpTransfer *transfer) {
        return postFile(m_url, transfer, "blob");
    }

    future<LegacyBlob> BlobAPI::uploadImage(std::string const& fileName, public_key const& pk, private_key const& sk) {
        byte_vector data;
        std::ifstream file;
        file.open(fileName, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("Unable to open file");
        }
        auto size = file.tellg();
        data.resize(size);
        file.seekg(0);
        file.read(reinterpret_cast<char*>(data.data()), size);
        file.close();

        return uploadData(data, BlobType::IMAGE, pk, sk);
    }

    future<void> BlobAPI::downloadFile(IHttpTransfer* transfer, blob_id const& id) {
        return downloadData(transfer, id);
    }

    future<void> BlobAPI::downloadImage(std::string const& fileName, LegacyBlob const& blob, public_key const& pk, private_key const& sk) {
        std::string url = BlobAPI::getDownloadURL(blob.id, m_useTLS);

        std::ofstream file;
        file.open(fileName, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Unable to open file");
        }

        return downloadData(blob, pk, sk).next([file{std::move(file)}](future<byte_vector> fut) mutable {
            byte_vector data = fut.get();
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
            if (!file.good()) {
                throw std::runtime_error("Unable to write file");
            }
        });
    }

    future<void> BlobAPI::deleteBlob(blob_id const& id) {
        std::string url = BlobAPI::getDownloadURL(id, m_useTLS);
        url += "/done";

        return get(url).next([](future<byte_vector> fut) {
            fut.get();
            return;
        });
    }

    future<LegacyBlob> BlobAPI::uploadData(byte_vector& data, BlobType type, public_key const& pk, private_key const& sk) {
        LegacyBlob blob;
        blob.n = crypto::generate_nonce();

        encrypt(data, blob.n, pk, sk);

        return postFile(m_url, data, "blob").next([blob{std::move(blob)}](future<byte_vector> fut) mutable {
            byte_vector blobString = fut.get();

            hex_decode(blobString, blob.id);
            LOG_DEBUG(logging::loggerRoot, "Uploaded data as " << blob.id);

            return blob;
        });
    }

    future<void> BlobAPI::downloadData(IHttpTransfer* transfer, blob_id const& id) {
        std::string url = BlobAPI::getDownloadURL(id, m_useTLS);

        return get(url, transfer);
    }

    future<byte_vector> BlobAPI::downloadData(LegacyBlob const& blob, public_key const& pk, private_key const& sk) {
        std::string url = BlobAPI::getDownloadURL(blob.id, m_useTLS);

        return get(url).next([blob, pk{pk}, sk{sk}](future<byte_vector> fut){
            byte_vector data = fut.get();
            if (!decrypt(data, blob.n, pk, sk)) {
                throw std::runtime_error("Unable to decrypt file");
            }

            return data;

        });
    }

    nonce const& BlobAPI::getFixedNonce(BlobType type) {
        switch(type) {
            case BlobType::IMAGE:
                throw std::runtime_error("Images do not have fixed nonces");
            case BlobType::VIDEO:
                return nonceVideo;
            case BlobType::VIDEO_THUMB:
                return nonceVideoThumb;
            case BlobType::AUDIO:
                return nonceAudio;
            case BlobType::FILE:
                return nonceFile;
            case BlobType::FILE_THUMB:
                return nonceFileThumb;
            case BlobType::GROUP_IMAGE:
                return nonceGroupImage;
            case BlobType::GROUP_ICON:
                return nonceGroupIcon;
            case BlobType::ICON:
                return nonceIcon;
        }
        throw std::domain_error("");
    }

    std::string BlobAPI::getDownloadURL(blob_id const& id, bool useTLS) {
        //https://%s.blob.threema.ch/%s
        std::string url;
        if (useTLS) {
            url = "https://";
        } else {
            url = "http://";
        }
        std::string blobString = hex_encode(id);
        url.append(blobString.begin(), blobString.begin()+2);
        url += ".blob.threema.ch/";
        url += blobString;
        //LOG_DEBUG(logging::loggerRoot, "Download url " << url);
        return url;
    }

    nonce BlobAPI::nonceVideo{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    nonce BlobAPI::nonceVideoThumb{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2};
    nonce BlobAPI::nonceAudio{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    nonce BlobAPI::nonceFile{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    nonce BlobAPI::nonceFileThumb{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2};
    nonce BlobAPI::nonceGroupImage{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    nonce BlobAPI::nonceGroupIcon{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    nonce BlobAPI::nonceIcon{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};

}
