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

/**
 * Like std::copy, but returns the advanced input iterator,
 * and copies into output.
 * It copies output.size() elements
 * Usually useful for deserialization.
 * @tparam It Input iterator type
 * @tparam Array Output array type
 * @param input Input iterator
 * @param output Output array with known size
 * @return Advanced input iterator
 */
template<typename It, typename Array>
It copy_iter(It input, Array& output) {
    std::copy(input, input + output.size(), output.begin());
    return input + output.size();
}