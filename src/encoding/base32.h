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

    static const std::array<char, 33> b32_chars = {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
            'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '2', '3', '4', '5', '6', '7',
            '=' };

    static const char b32_table[128] = {
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 26, 27, 28, 29, 30, 31, 64, 64, 64, 64, 64, 64, 64, 64,
            64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
            64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    };

    /**
     * Decode Base32 encoded string into buffer, either until buffer is full, or string
     * is exhausted. Non-base32 characters are ignored
     * @param from String to decode, must represent a multiple of 40 bits.
     * @param to Buffer to write to
     * @param len Size of buffer in bytes
     * @return Length of decoded buffer in bytes, or -1 on error (invalid input length)
     */
    inline int base32_decode(std::string const &from, unsigned char *to, std::size_t len) {
        unsigned int j = 0, bits = 0, value = 0;
        for (char ch: from) {
            if (ch < 0 || ch > 127 || b32_table[static_cast<unsigned char>(ch)] == 64) {
                // Skip invalid char
                continue;
            }
            value = (value << 5) | b32_table[static_cast<unsigned char>(ch)];
            bits += 5;
            if (bits >= 8) {
                // Read top 8 bits
                bits -= 8;
                to[j++] = static_cast<unsigned char>((value >> (bits)) & 0xFF);
                if (j > len) {
                    // Buffer filled to capacity
                    return j;
                }
            }
        }
        if (bits != 0) {
            // Invalid string length
            return -1;
        }

        // String exhausted
        return j;
    }

    template<typename Array>
    inline std::string base32_encode(Array const& from) {
        byte_vector to;
        int pad = from.size() % 5;
        to.resize(from.size() / 5 * 8 + (pad?8:0), 32);

        size_t i = 0, j = 0;
        for(; i < from.size()-4; i += 5, j += 8) {
            to[j] = from[i] >> 3;
            to[j+1] = (from[i] << 2 | from[i+1] >> 6) & 0x1f;
            to[j+2] = from[i+1] >> 1 & 0x1f;
            to[j+3] = (from[i+1] << 4 | from[i+2] >> 4) & 0x1f;
            to[j+4] = (from[i+2] << 1 | from[i+3] >> 7) & 0x1f;
            to[j+5] = from[i+3] >> 2 & 0x1f;
            to[j+6] = (from[i+3] << 3 | from[i+4] >> 5) & 0x1f;
            to[j+7] = from[i+4] & 0x1f;
        }
        if (pad) {
            to[j] = from[i] >> 3;
            if (pad != 1) {
                to[j+1] = (from[i] << 2 | from[i+1] >> 6) & 0x1f;
                to[j+2] = from[i+1] >> 1 & 0x1f;
                if (pad != 2) {
                    to[j+3] = (from[i+1] << 4 | from[i+2] >> 4) & 0x1f;
                    if (pad != 3) {
                        to[j+4] = (from[i+2] << 1 | from[i+3] >> 7) & 0x1f;
                        to[j+5] = from[i+3] >> 2 & 0x1f;
                        if (pad != 4) {
                            to[j+6] = (from[i+3] << 3 | from[i+4] >> 5) & 0x1f;
                        } else {
                            to[j+6] = from[i+3] << 3 & 0x1f;
                        }
                    } else {
                        to[j+4] = from[i+2] << 1 & 0x1f;
                    }
                } else {
                    to[j+3] = from[i+1] << 4 & 0x1f;
                }

            } else {
                to[j+1] = from[i] << 2 & 0x1f;
            }

        }

        //TODO: copy just because of signedness
        //otherwise replace inplace
        std::string result;
        result.reserve(to.size());
        for(auto v: to) {
            result += b32_chars[v];
        }
        return result;
    }
}
