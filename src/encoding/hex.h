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

#include <array>
#include <types/bytes.h>

namespace ceema {
    static const std::array<char, 16> hex_chars = {
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',};

    static const std::array<std::uint8_t, 128> hex_table = {
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 64,
             0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 64, 64, 64, 64, 64, 64,
            64, 10, 11, 12, 13, 14, 15, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 10, 11, 12, 13, 14, 15, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    };

    template<typename ArrayIn, typename ArrayOut>
    inline void hex_encode(ArrayIn const& from, ArrayOut& to) {
        if (to.size() / 2 != from.size()) {
            throw std::runtime_error("Invalid hex output size");
        }
        for(std::size_t i = 0; i < from.size(); i++) {
            to[i*2] = hex_chars[from[i] >> 4];
            to[i*2+1] = hex_chars[from[i] & 0x0f];
        }
    }

    template<typename Array>
    inline std::string hex_encode(Array const& from) {
        std::string to;
        to.resize(from.size() * 2);
        hex_encode(from, to);
        return to;
    }

    template<typename ArrayIn, typename ArrayOut>
    inline void hex_decode(ArrayIn const& from, ArrayOut& to) {
        if (from.size() % 2 != 0) {
            throw std::runtime_error("Invalid hex input size ");
        }
        if (to.size() * 2 != from.size()) {
            throw std::runtime_error("Invalid output size");
        }
        for(std::size_t i = 0; i < from.size() - 1; i += 2) {
            auto hi_char = from[i];
            auto lo_char = from[i+1];
            if ((hi_char > 127) || (hi_char < 0) || (hex_table[hi_char] == 64) ||
                (lo_char > 127) || (lo_char < 0) || (hex_table[lo_char] == 64)) {
                throw std::runtime_error("Invalid hex input");
            }
            auto hi_val = hex_table[hi_char];
            auto lo_val = hex_table[lo_char];
            to[i/2] = hi_val << 4 | lo_val;
        }
    }

    template<typename Array>
    inline byte_vector hex_decode(Array const& from) {
        byte_vector res;
        res.resize(from.size() / 2);
        hex_decode(from, res);
        return res;
    }

    template<typename ArrayOut, typename ArrayIn>
    inline ArrayOut hex_decode(ArrayIn const& from) {
        ArrayOut res;
        hex_decode(from, res);
        return res;
    }

}
