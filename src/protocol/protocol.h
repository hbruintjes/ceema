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

#include "types/bytes.h"

namespace ceema {

    template<typename T>
    inline void htole(T const& value, std::uint8_t* buffer) {
#ifdef PLATFORM_BIG_ENDIAN
        //DO thing
#error "Big Endian platform not yet supported"
#else
        *reinterpret_cast<T*>(buffer) = value;
#endif
    }

    template<typename T>
    inline void letoh(T& value, std::uint8_t const* buffer) {
#ifdef PLATFORM_BIG_ENDIAN
        //DO thing
#error "Big Endian platform not yet supported"
#else
        value = *reinterpret_cast<T const*>(buffer);
#endif
    }

    template<typename T>
    inline T letoh(std::uint8_t const* buffer) {
#ifdef PLATFORM_BIG_ENDIAN
        //DO thing
#error "Big Endian platform not yet supported"
#else
        return *reinterpret_cast<T const*>(buffer);
#endif
    }

}
