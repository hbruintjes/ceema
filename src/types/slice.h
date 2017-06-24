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

    template<typename Array, std::size_t offset, std::size_t length>
    struct slice {
        typedef typename Array::value_type value_type;
        typedef value_type *pointer;
        typedef const value_type *const_pointer;
        typedef value_type &reference;
        typedef const value_type &const_reference;
        typedef value_type *iterator;
        typedef const value_type *const_iterator;
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef std::reverse_iterator <iterator> reverse_iterator;
        typedef std::reverse_iterator <const_iterator> const_reverse_iterator;

        Array &m_array;

        slice(Array &array) : m_array(array) {}

        void fill(const value_type &__u) { std::fill_n(begin(), size(), __u); }

        void
        swap(slice &__other) noexcept { std::swap_ranges(begin(), end(), __other.begin()); }

        // Iterators.
        iterator
        begin() noexcept { return iterator(data()); }

        const_iterator
        begin() const noexcept { return const_iterator(data()); }

        iterator
        end() noexcept { return iterator(data()) + length; }

        const_iterator
        end() const noexcept { return const_iterator(data()) + length; }

        reverse_iterator
        rbegin() noexcept { return reverse_iterator(end()); }

        const_reverse_iterator
        rbegin() const noexcept { return const_reverse_iterator(end()); }

        reverse_iterator
        rend() noexcept { return reverse_iterator(begin()); }

        const_reverse_iterator
        rend() const noexcept { return const_reverse_iterator(begin()); }

        const_iterator
        cbegin() const noexcept { return const_iterator(data()); }

        const_iterator
        cend() const noexcept { return const_iterator(data() + length); }

        const_reverse_iterator
        crbegin() const noexcept { return const_reverse_iterator(end()); }

        const_reverse_iterator
        crend() const noexcept { return const_reverse_iterator(begin()); }

        // Capacity.
        constexpr size_type
        size() const noexcept { return length; }

        constexpr size_type
        max_size() const noexcept { return length; }

        constexpr bool
        empty() const noexcept { return size() == 0; }

        // Element access.
        reference
        operator[](size_type __n) { return m_array[__n + offset]; }

        constexpr const_reference
        operator[](size_type __n) const noexcept { return m_array[__n + offset]; }

        reference
        at(size_type __n) {
            return m_array.at(__n + offset);
        }

        constexpr const_reference
        at(size_type __n) const {
            return m_array.at(__n + offset);
        }

        reference
        front() { return *begin(); }

        constexpr const_reference
        front() const { return *begin(); }

        reference
        back() { return length ? *(end() - 1) : *end(); }

        constexpr const_reference
        back() const { return length ? *(end() - 1) : *end(); }

        pointer
        data() noexcept { return m_array.data() + offset; }

        const_pointer
        data() const noexcept { return m_array.data() + offset; }
    };

    template<typename Array, std::size_t offset, std::size_t length>
    struct slice<Array const, offset, length> {
        typedef typename Array::value_type const value_type;
        typedef value_type *pointer;
        typedef const value_type *const_pointer;
        typedef value_type &reference;
        typedef const value_type &const_reference;
        typedef value_type *iterator;
        typedef const value_type *const_iterator;
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef std::reverse_iterator <iterator> reverse_iterator;
        typedef std::reverse_iterator <const_iterator> const_reverse_iterator;

        Array const& m_array;

        slice(Array const& array) : m_array(array) {}

        // Iterators.
        const_iterator
        begin() const noexcept { return const_iterator(data()); }

        const_iterator
        end() const noexcept { return const_iterator(data()) + length; }

        const_reverse_iterator
        rbegin() const noexcept { return const_reverse_iterator(end()); }

        const_reverse_iterator
        rend() const noexcept { return const_reverse_iterator(begin()); }

        const_iterator
        cbegin() const noexcept { return const_iterator(data()); }

        const_iterator
        cend() const noexcept { return const_iterator(data() + length); }

        const_reverse_iterator
        crbegin() const noexcept { return const_reverse_iterator(end()); }

        const_reverse_iterator
        crend() const noexcept { return const_reverse_iterator(begin()); }

        // Capacity.
        constexpr size_type
        size() const noexcept { return length; }

        constexpr size_type
        max_size() const noexcept { return length; }

        constexpr bool
        empty() const noexcept { return size() == 0; }

        // Element access.
        reference
        operator[](size_type __n) { return m_array[__n + offset]; }

        constexpr const_reference
        operator[](size_type __n) const noexcept { return m_array[__n + offset]; }

        constexpr const_reference
        at(size_type __n) const {
            return m_array.at(__n + offset);
        }

        constexpr const_reference
        front() const { return *begin(); }

        constexpr const_reference
        back() const { return length ? *(end() - 1) : *end(); }

        const_pointer
        data() const noexcept { return m_array.data() + offset; }
    };

    template<std::size_t offset, std::size_t length, typename Array>
    slice<Array, offset, length> slice_array(Array &array) {
        return slice<Array, offset, length>(array);
    }

// Slice comparisons.
    template<typename Array, std::size_t offset, std::size_t length>
    inline bool
    operator==(const slice<Array, offset, length> &__one, const slice<Array, offset, length> &__two) {
        return std::equal(__one.begin(), __one.end(), __two.begin());
    }

    template<typename Array, std::size_t offset, std::size_t length>
    inline bool
    operator!=(const slice<Array, offset, length> &__one, const slice<Array, offset, length> &__two) {
        return !(__one == __two);
    }


}
