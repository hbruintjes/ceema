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

#include <protocol/protocol.h>
#include <encoding/hex.h>
#include <types/iter.h>
#include "PayloadBlob.h"
#include <json/src/json.hpp>

using nlohmann::json;

namespace ceema {

    constexpr unsigned PayloadPictureSize = blob_id::array_size + sizeof(blob_size) + nonce::array_size;

    PayloadPicture PayloadPicture::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size != PayloadPictureSize) {
            throw std::runtime_error("Invalid picture payload");
        }

        PayloadPicture payload;
        payload_data = copy_iter(payload_data, payload.id);
        letoh(payload.size, &*payload_data);
        payload_data += sizeof(payload.size);
        payload_data = copy_iter(payload_data, payload.n);
        return payload;
    }

    byte_vector PayloadPicture::serialize() {
        byte_vector res;
        res.resize(PayloadPictureSize);

        auto iter = res.begin();
        iter = std::copy(id.begin(), id.end(), iter);
        htole(size, &*iter);
        iter += sizeof(size);
        iter = std::copy(n.begin(), n.end(), iter);

        return res;
    }

    constexpr unsigned PayloadVideoSize = sizeof(std::uint16_t) + blob_id::array_size + sizeof(blob_size) +
            blob_id::array_size + sizeof(blob_size) + shared_key::array_size;

    PayloadVideo PayloadVideo::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size != PayloadVideoSize) {
            throw std::runtime_error("Invalid video payload");
        }

        PayloadVideo payload;
        letoh(payload.m_duration, &*payload_data);
        payload_data += sizeof(payload.m_duration);

        payload_data = copy_iter(payload_data, payload.id);
        letoh(payload.size, &*payload_data);
        payload_data += sizeof(payload.size);

        payload_data = copy_iter(payload_data, payload.thumb_id);
        letoh(payload.thumb_size, &*payload_data);
        payload_data += sizeof(payload.thumb_size);

        payload_data = copy_iter(payload_data, payload.key);

        return payload;
    }

    byte_vector PayloadVideo::serialize() {
        byte_vector res;
        res.resize(PayloadVideoSize);

        auto iter = res.begin();
        htole(m_duration, &*iter);
        iter += 2;
        iter = std::copy(id.begin(), id.end(), iter);
        htole(size, &*iter);
        iter += sizeof(size);

        iter = std::copy(thumb_id.begin(), thumb_id.end(), iter);
        htole(thumb_size, &*iter);
        iter += sizeof(thumb_size);

        iter = std::copy(key.begin(), key.end(), iter);

        return res;
    }

    constexpr unsigned PayloadAudioSize = sizeof(std::uint16_t) + blob_id::array_size + sizeof(blob_size) +
                                          shared_key::array_size;

    PayloadAudio PayloadAudio::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size != PayloadAudioSize) {
            throw std::runtime_error("Invalid audio payload");
        }

        PayloadAudio payload;
        letoh(payload.m_duration, &*payload_data);
        payload_data += sizeof(payload.m_duration);

        payload_data = copy_iter(payload_data, payload.id);
        letoh(payload.size, &*payload_data);
        payload_data += sizeof(payload.size);
        payload_data = copy_iter(payload_data, payload.key);
        return payload;
    }

    byte_vector PayloadAudio::serialize() {
        byte_vector res;
        res.resize(PayloadAudioSize);

        auto iter = res.begin();
        htole(m_duration, &*iter);
        iter += 2;
        iter = std::copy(id.begin(), id.end(), iter);
        htole(size, &*iter);
        iter += sizeof(size);
        iter = std::copy(key.begin(), key.end(), iter);

        return res;
    }

    /**
     * Function to decode file JSON data into PayloadFile
     * @param j JSON data
     * @param f Structure to write parsed result into
     */
    void from_json(json const& j, PayloadFile& f) {
        hex_decode(j["b"].get<std::string>(), f.id);
        if (j.count("t")) {
            hex_decode(j["t"].get<std::string>(), f.thumb_id);
            f.has_thumb = true;
        } else {
            f.has_thumb = false;
        }
        hex_decode(j["k"].get<std::string>(), f.key);
        f.mime_type = j["m"];
        f.filename = j["n"];
        f.size = j["s"];
        std::uint8_t val = j["i"];
        f.flag = static_cast<PayloadFile::FileFlag>(val);
        if (j.count("d")) {
            f.caption = j["d"];
        } else {
            f.caption = "";
        }
    }

    /**
     * Function to encode PayloadFile data into JSON
     * @param j JSON structureto write into
     * @param f Payload to encode
     */
    void to_json(json& j, PayloadFile const& f) {
        j["b"] = hex_encode(f.id);
        if (f.has_thumb) {
            j["t"] = hex_encode(f.thumb_id);
        }
        j["k"] = hex_encode(f.key);
        j["m"] = f.mime_type;
        j["n"] = f.filename;
        j["s"] = f.size;
        j["i"] = static_cast<std::uint8_t>(f.flag);
        if (f.caption.size()) {
            j["d"] = f.caption;
        }
    }

    PayloadFile PayloadFile::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size < 1) {
            throw std::runtime_error("Invalid file payload");
        }
        json val = json::parse(payload_data, payload_data + size);
        return val.get<PayloadFile>();
    }

    byte_vector PayloadFile::serialize() {
        byte_vector res;
        json file = json(*this);
        std::string jsonString = file.dump();
        res.insert(res.end(), jsonString.begin(), jsonString.end());

        return res;
    }

    constexpr unsigned PayloadIconSize = blob_id::array_size + sizeof(blob_size) + nonce::array_size;

    PayloadIcon PayloadIcon::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size != PayloadPictureSize) {
            throw std::runtime_error("Invalid icon payload");
        }

        PayloadIcon payload;
        payload_data = copy_iter(payload_data, payload.id);
        letoh(payload.size, &*payload_data);
        payload_data += sizeof(payload.size);
        payload_data = copy_iter(payload_data, payload.n);
        return payload;
    }

    byte_vector PayloadIcon::serialize() {
        byte_vector res;
        res.resize(PayloadPictureSize);

        auto iter = res.begin();
        iter = std::copy(id.begin(), id.end(), iter);
        htole(size, &*iter);
        iter += sizeof(size);
        iter = std::copy(n.begin(), n.end(), iter);

        return res;
    }
}