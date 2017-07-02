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

#include <protocol/packet/messagetypes.h>
#include <protocol/data/Blob.h>
#include <types/bytes.h>
#include <string>
#include <protocol/packet/MessageFlag.h>

namespace ceema {

    /**
     * Picture is sent via blob, encrypted using recipient public key
     * with provided nonce.
     */
    struct PayloadPicture {
        static constexpr MessageType Type = MessageType::PICTURE;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{MessageFlag::PUSH};
        }

        /** ID of blob containing data */
        blob_id id;
        /** Size of blob in bytes */
        blob_size size;
        /** Nonce used to encrypt the blob with */
        nonce n;

        static PayloadPicture deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize();
    };



    /**
     * Video is sent via two blobs (thumbnail and file), with a shared key.
     * The nonce is fixed, the key is generated.
     */
    struct PayloadVideo {
        static constexpr MessageType Type = MessageType::VIDEO;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{MessageFlag::PUSH};
        }

        /** Video play time in seconds */
        std::uint16_t m_duration;
        /** Blob containing video data */
        blob_id id;
        /** size in bytes */
        blob_size size;
        /** Blob containing video thumbnail data */
        blob_id thumb_id;
        /** thumbnail size in bytes */
        blob_size thumb_size;
        /** encryption key */
        shared_key key;

        static PayloadVideo deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize();
    };

    /**
     * Audio is sent as a single blob with shared key.
     */
    struct PayloadAudio {
        static constexpr MessageType Type = MessageType::AUDIO;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{MessageFlag::PUSH};
        }

        /** Audio play time in seconds */
        std::uint16_t m_duration;
        /** Blob containing audio data */
        blob_id id;
        /** size in bytes */
        blob_size size;
        /** encryption key */
        shared_key key;

        static PayloadAudio deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize();
    };

    /**
     * Generic file transfer with support for optional thumbnails and captions. Nonces are
     * fixed, keys are generated. Note that the filesize of thumbnails is not transmitted
     * in advance.
     * File data is sent as JSON configuration mostly
     */
    struct PayloadFile {
        /**
         * File transfer flag. Unknown use.
         */
        enum class FileFlag : std::uint8_t {
            DEFAULT = 0x00,  // Default transfer flag
            IN_APP_MESSAGE = 0x01  // Unknown transfer flag
        };

        static constexpr MessageType Type = MessageType::FILE;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{MessageFlag::PUSH};
        }

        /** Blob containing file data */
        blob_id id;
        /** size in bytes */
        blob_size size;
        /** encryption key */
        shared_key key;

        /** Blob containing thumbnail data (if has_thumb) */
        blob_id thumb_id;

        /** True if a thumbnail was provided (otherwise, thumb_id and thumb_size are invalid) */
        bool has_thumb;

        /** Name ofthe file */
        std::string filename;
        /** Type of the file */
        std::string mime_type;
        /** Optional caption of transfer */
        std::string caption;
        /** Transfer flag. Current only DEFAULT supported */
        FileFlag flag;

        static PayloadFile deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize();
    };

    /**
     * Icon is sent via blob, encrypted using recipient public key
     * with provided nonce.
     */
    struct PayloadIcon {
        static constexpr MessageType Type = MessageType::ICON;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{};
        }

        /** ID of blob containing data */
        blob_id id;
        /** Size of blob in bytes */
        blob_size size;
        /** Key used to encrypt the blob with */
        shared_key key;

        static PayloadIcon deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize();
    };

    /**
     * Request to clear the icon of a user
     */
    struct PayloadIconClear {
        static constexpr MessageType Type = MessageType::ICON_CLEAR;
        static /*constexpr*/ MessageFlags default_flags() {
            return MessageFlags{};
        }

        static PayloadIconClear deserialize(byte_vector::const_iterator& payload_data, std::size_t size);
        byte_vector serialize();
    };
}