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

#include <cstddef>
#include <iterator>

namespace ceema {

    template<typename T>
    struct ptr_array {
        typedef T value_type;
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef value_type &reference;
        typedef const value_type &const_reference;
        typedef pointer iterator;
        typedef const_pointer const_iterator;
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef std::reverse_iterator <iterator> reverse_iterator;
        typedef std::reverse_iterator <const_iterator> const_reverse_iterator;

        iterator m_begin;
        iterator m_end;

        ptr_array(iterator begin, size_type size) : m_begin(begin), m_end(begin + size) {}

        void fill(const value_type &__u) { std::fill_n(begin(), size(), __u); }

        void
        swap(ptr_array& other) noexcept {
            std::swap(m_begin, other.m_begin);
            std::swap(m_end, other.m_end);
        }

        // Iterators.
        iterator
        begin() noexcept { return iterator(m_begin); }

        const_iterator
        begin() const noexcept { return const_iterator(m_begin); }

        iterator
        end() noexcept { return iterator(m_end); }

        const_iterator
        end() const noexcept { return const_iterator(m_end); }

        reverse_iterator
        rbegin() noexcept { return reverse_iterator(end()); }

        const_reverse_iterator
        rbegin() const noexcept { return const_reverse_iterator(end()); }

        reverse_iterator
        rend() noexcept { return reverse_iterator(begin()); }

        const_reverse_iterator
        rend() const noexcept { return const_reverse_iterator(begin()); }

        const_iterator
        cbegin() const noexcept { return const_iterator(begin()); }

        const_iterator
        cend() const noexcept { return const_iterator(end()); }

        const_reverse_iterator
        crbegin() const noexcept { return const_reverse_iterator(end()); }

        const_reverse_iterator
        crend() const noexcept { return const_reverse_iterator(begin()); }

        // Capacity.
        size_type
        size() const noexcept { return m_end - m_begin; }

        size_type
        max_size() const noexcept { return size(); }

        bool
        empty() const noexcept { return size() == 0; }

        // Element access.
        reference
        operator[](size_type __n) { return *(begin() + __n); }

        constexpr const_reference
        operator[](size_type __n) const noexcept { return *(cbegin() + __n); }

        reference
        at(size_type __n) {
            return *(begin() + __n);
        }

        const_reference
        at(size_type __n) const {
            return *(cbegin() + __n);
        }

        reference
        front() { return *begin(); }

        const_reference
        front() const { return *begin(); }

        reference
        back() { return size() ? *(end() - 1) : *end(); }

        const_reference
        back() const { return size() ? *(end() - 1) : *end(); }

        pointer
        data() noexcept { return begin(); }

        const_pointer
        data() const noexcept { return cbegin(); }
    };


}
