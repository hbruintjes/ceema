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

#include "MessagePayload.h"
#include <protocol/protocol.h>
#include <logging/logging.h>
#include <types/bytes.h>

namespace ceema {

    template<typename It, typename Array>
    It copy_iter(It input, Array& output) {
        std::copy(input, input + Array::array_size, output.begin());
        return input + Array::array_size;
    }

    MessagePayload MessagePayload::deserialize(byte_vector const& payload) {
        byte_vector::const_iterator payload_data = payload.begin();
        MessageType type;
        letoh(type, &*payload_data);
        payload_data += sizeof(MessageType);
        auto size = payload.size() - sizeof(MessageType);
        switch(type) {
            case MessageType::TEXT:
                return MessagePayload(PayloadText::deserialize(payload_data, size));
            case MessageType::LOCATION:
                return MessagePayload(PayloadLocation::deserialize(payload_data, size));
            case MessageType::PICTURE:
                return MessagePayload(PayloadPicture::deserialize(payload_data, size));
            case MessageType::VIDEO:
                return MessagePayload(PayloadVideo::deserialize(payload_data, size));
            case MessageType::AUDIO:
                return MessagePayload(PayloadAudio::deserialize(payload_data, size));
            case MessageType::POLL:
                return MessagePayload(PayloadPoll::deserialize(payload_data, size));
            case MessageType::POLL_VOTE:
                return MessagePayload(PayloadPollVote::deserialize(payload_data, size));
            case MessageType::MESSAGE_STATUS:
                return MessagePayload(PayloadMessageStatus::deserialize(payload_data, size));
            case MessageType::CLIENT_TYPING:
                return MessagePayload(PayloadTyping::deserialize(payload_data, size));
            case MessageType::FILE:
                return MessagePayload(PayloadFile::deserialize(payload_data, size));
            case MessageType::GROUP_MEMBERS:
                return MessagePayload(PayloadGroupMembers::deserialize(payload_data, size));
            case MessageType::GROUP_TITLE:
                return MessagePayload(PayloadGroupTitle::deserialize(payload_data, size));
            case MessageType::GROUP_SYNC:
                return MessagePayload(PayloadGroupSync::deserialize(payload_data, size));
            case MessageType::GROUP_TEXT:
                return MessagePayload(PayloadGroupText::deserialize(payload_data, size));
            case MessageType::GROUP_LOCATION:
                return MessagePayload(PayloadGroupLocation::deserialize(payload_data, size));
            case MessageType::GROUP_PICTURE:
                return MessagePayload(PayloadGroupPicture::deserialize(payload_data, size));
//            case MessageType::GROUP_VIDEO:
//                return MessagePayload(PayloadGroupVideo::deserialize(payload_data, size));
//            case MessageType::GROUP_AUDIO:
//                return MessagePayload(PayloadGroupAudio::deserialize(payload_data, size));
            case MessageType::GROUP_FILE:
                return MessagePayload(PayloadGroupFile::deserialize(payload_data, size));
//            case MessageType::GROUP_POLL:
//                return MessagePayload(PayloadGroupPoll::deserialize(payload_data, size));
//            case MessageType::GROUP_POLL_VOTE:
//                return MessagePayload(PayloadGroupPollVote::deserialize(payload_data, size));
//            case MessageType::GROUP_ICON:
//                return MessagePayload(PayloadGroupIcon::deserialize(payload_data, size));
//            case MessageType::GROUP_LEAVE:
//                return MessagePayload(PayloadGroupLeave::deserialize(payload_data, size));
            default:
                throw std::runtime_error("Invalid type for MessagePayload " + std::to_string(static_cast<unsigned>(type)));
        }
    }

    byte_vector PayloadNone::serialize() {
        throw std::logic_error("Cannot serialize NONE message");
    }


}
