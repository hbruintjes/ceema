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

#include "slice.h"

#include <sodium.h>

#include <cstdint>
#include <array>
#include <vector>
#include <iostream>
#include <iomanip>

namespace ceema {

    /** Simple vector for bytes */
    typedef std::vector<std::uint8_t> byte_vector;

    /** Simple fixed size byte array */
    template<std::size_t N>
    struct byte_array : public std::array<std::uint8_t, N> {
        static constexpr std::size_t array_size = N;

        byte_array() = default;

        byte_array(byte_array const &) = default;

        byte_array(byte_array &&) = default;

        template<typename Array, std::size_t Offset, std::size_t L>
        explicit byte_array(slice<Array, Offset, L> const& slice) {
            std::copy(slice.begin(), slice.end(), this->begin());
        }
        template<typename Array, std::size_t Offset, std::size_t L>
        explicit byte_array(slice<Array, Offset, L> && slice) {
            std::copy(slice.begin(), slice.end(), this->begin());
        }

        explicit byte_array(byte_vector const& vec) {
            auto size = std::min(N, vec.size());
            std::copy(vec.begin(), vec.begin() + size, this->begin());
        }
        explicit byte_array(byte_vector&& vec) {
            auto size = std::min(N, vec.size());
            std::copy(vec.begin(), vec.begin() + size, this->begin());
        }

        byte_array& operator=(byte_array const &) = default;

        byte_array& operator=(byte_array &&) = default;

        template<typename Array, std::size_t Offset>
        byte_array& operator=(slice<Array, Offset, N> const& slice) {
            std::copy(slice.begin(), slice.end(), this->begin());
            return *this;
        }

        template<typename... Args>
        explicit byte_array(Args const&&...args) : std::array<std::uint8_t, N>{{std::forward<std::uint8_t>(args)...}} {}
    };

}


namespace std {

    template<size_t N>
    inline ostream &operator<<(ostream &os, ceema::byte_array<N> const &array) {
        auto flags = os.setf(ios::hex);
        auto fill = os.fill('0');
        for (auto const &element: array) {
            os << hex << setw(2) << static_cast<unsigned>(element);
        }
        os.flags(flags);
        os.fill(fill);
        return os;
    }

    template<typename _CharT, typename _Traits>
    inline basic_ostream<_CharT, _Traits>&
    operator<<(basic_ostream<_CharT, _Traits>& os, ceema::byte_vector const &vec) {
        auto flags = os.setf(ios::hex);
        auto fill = os.fill('0');
        for (auto const &element: vec) {
            os << hex << setw(2) << static_cast<unsigned>(element);
        }
        os.flags(flags);
        os.fill(fill);
        return os;
    }

    template<typename A, size_t O, size_t L>
    inline typename enable_if<is_same<typename A::value_type, uint8_t>::value, ostream &>::type
    operator<<(ostream &os, ceema::slice<A, O, L> const& slice) {
        auto flags = os.setf(ios::hex);
        auto fill = os.fill('0');
        for (auto const &element: slice) {
            os << hex << setw(2) << (unsigned int) element;
        }
        os.flags(flags);
        os.fill(fill);
        return os;
    }

    template<typename A, size_t O, size_t L>
    inline typename enable_if<!is_same<typename A::value_type, uint8_t>::value, ostream &>::type
    operator<<(ostream &os, ceema::slice <A, O, L> const& slice) {
        for (auto const &element: slice) {
            os << element;
        }
        return os;
    }


}
