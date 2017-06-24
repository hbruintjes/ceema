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

#include <string>
#include <logging/logging.h>

namespace ceema {
    static const std::array<char, 64> b64_chars = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

    static const std::array<char, 128> b64_table = {
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
            52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
            64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
            64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64
    };

    template<typename Array>
    inline std::string base64_encode(Array const& from) {
        static_assert(std::is_unsigned<typename Array::value_type>::value,
                      "Cannot encode signed types");
        std::string to;
        size_t from_len = from.size();
        size_t pad = from_len % 3;
        // Calculate output size

        size_t outlen = ((from_len / 3) + (pad ? 1 : 0)) * 4;

        to.resize(outlen, '=');

        size_t i = 0, j = 0;
        for(; i < from_len - 2; i += 3, j += 4) {
            to[j] = b64_chars[from[i] >> 2 & 0x3f];
            to[j+1] = b64_chars[(from[i] << 4 | from[i+1] >> 4) & 0x3f];
            to[j+2] = b64_chars[(from[i+1] << 2 | from[i+2] >> 6) & 0x3f];
            to[j+3] = b64_chars[from[i+2] & 0x3f];
        }
        if (pad) {
            to[j] = b64_chars[from[i] >> 2 & 0x3f];
            to[j+1] = b64_chars[(from[i] << 4 | from[i+1] >> 4) & 0x3f];
            if (pad == 2) {
                to[j+2] = b64_chars[(from[i+1] << 2 | from[i+2] >> 6) & 0x3f];
            }
        }
        return to;
    }

    /**
     * Decode Base64 encoded string into buffer, either until buffer is full, or string
     * is exhausted. Non-base64 characters are ignored
     * @param from String to decode, should represent a multiple of 3 bytes (padding is optional).
     * @param to Buffer to write to
     * @param len Size of buffer in bytes
     * @return Length of decoded buffer in bytes
     */
    inline int base64_decode(std::string const &from, unsigned char *to, std::size_t len) {
        unsigned int j = 0, bits = 0, value = 0;
        for (char c: from) {
            if ((c > 127) || (c < 0) || (b64_table[c] > 63)) {
                // Skip invalid char
                continue;
            }
            value = (value << 6) | b64_table[c];
            bits += 6;
            if (bits >= 8) {
                // Read top 8 bits
                bits -= 8;
                to[j++] = static_cast<unsigned char>((value >> (bits)) & 0xFF);
                if (j > len) {
                    // Buffer filled to capacity
                    throw std::runtime_error("Base decode buffer too small");
                }
            }

        }
        if (bits != 0) {
            if (j < len) {
                to[j++] = static_cast<unsigned char>(value & (0xFF >> (8 - bits)));
            }
        }

        // String exhausted
        if (j != len) {
            throw std::runtime_error("Base decode buffer too large");
        }
        return j;
    }

    inline byte_vector base64_decode(std::string const &from) {
        byte_vector to;
        to.reserve(from.size() / 4 * 3); // Approximate result size, does not have to be exact

        for(std::size_t i = 0; i < from.size(); i += 4) {
            std::uint32_t val = (b64_table[from[i]]) << 18 | (b64_table[from[i+1]] << 12) | (b64_table[from[i+2]] << 6) | (b64_table[from[i+3]]);
            to.push_back(val >> 16);
            if (from[i+2] != '=') {
                to.push_back(val >> 8 & 0xff);
                if (from[i+3] != '=') {
                    to.push_back(val & 0xff);
                }
            }
        }

        // String exhausted
        return to;
    }

}
