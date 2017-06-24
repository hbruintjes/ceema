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

#include <cstdint>
#include <iosfwd>

namespace ceema {

    enum class MessageType : std::uint8_t {
        NONE = 0x00, // Internal ID

        TEXT = 0x01,
        PICTURE = 0x02,
        LOCATION = 0x10,
        ICON = 0x12,
        VIDEO = 0x13,
        AUDIO = 0x14,
        POLL = 0x15,
        POLL_VOTE = 0x16,
        FILE = 0x17,

        GROUP_TEXT = 0x41,
        GROUP_LOCATION = 0x42,
        GROUP_PICTURE = 0x43,
        GROUP_VIDEO = 0x44,
        GROUP_AUDIO = 0x45,
        GROUP_FILE = 0x46,

        GROUP_MEMBERS = 0x4A,
        GROUP_TITLE = 0x4B,
        GROUP_LEAVE = 0x4C,

        GROUP_ICON = 0x50,
        GROUP_SYNC = 0x51,

        GROUP_POLL = 0x52,
        GROUP_POLL_VOTE = 0x53,

        MESSAGE_STATUS = 0x80,
        CLIENT_TYPING = 0x90,
    };

    std::ostream& operator<<(std::ostream& os, MessageType type);
}
