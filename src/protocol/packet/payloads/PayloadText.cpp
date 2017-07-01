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

#include "PayloadText.h"

namespace ceema {
    PayloadText PayloadText::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size < 1) {
            throw std::runtime_error("Invalid text payload");
        }
        return PayloadText{std::string(reinterpret_cast<const char*>(&*payload_data), size)};
    }

    byte_vector PayloadText::serialize() {
        return byte_vector(m_text.begin(), m_text.end());
    }

    PayloadLocation PayloadLocation::deserialize(byte_vector::const_iterator& payload_data, std::size_t size) {
        if (size < 1) {
            throw std::runtime_error("Invalid location payload");
        }

        PayloadLocation payload;

        std::vector<std::string> lines;
        std::string payloadString = std::string(reinterpret_cast<const char*>(&*payload_data), size);
        std::string::size_type startpos = 0;
        std::string::size_type endpos;
        while(lines.size() < 2 && (endpos = payloadString.find('\n', startpos)) != std::string::npos) {
            lines.emplace_back(payloadString.begin()+startpos, payloadString.begin()+endpos);
            startpos = endpos+1;
        }
        if (startpos < payloadString.size()) {
            lines.emplace_back(payloadString.begin()+startpos, payloadString.end());
        }

        std::stringstream ss(lines[0]);
        ss >> payload.m_lattitude;
        if (ss.peek() == ',') {
            ss.ignore();
        } else {
            throw std::runtime_error("Invalid location payload");
        }
        ss >> payload.m_longitude;
        if (ss.peek() == ',') {
            ss.ignore();
            ss >> payload.m_accuracy;
        } else {
            payload.m_accuracy = 0.0;
        }

        if (lines.size() > 1) {
            payload.m_location = lines[1];
            if (lines.size() > 2) {
                payload.m_description = lines[2];
                while((endpos = payload.m_description.find("\\n")) != std::string::npos) {
                    payload.m_description.replace(endpos, 2, 1, '\n');
                }
            }
        }

        if (!ss) {
            throw std::runtime_error("Invalid location payload");
        }
        return payload;
    }

    byte_vector PayloadLocation::serialize() {
        std::stringstream ss;
        ss << m_lattitude << ',' << m_longitude;
        if (m_accuracy != 0.0) {
            ss << ',' << m_accuracy;
        }
        if (!m_location.empty()) {
            ss << '\n' << m_location;
            if (!m_description.empty()) {
                ss << '\n' << m_description;
            }
        }
        std::string str = ss.str();
        return byte_vector(str.begin(), str.end());
    }
}