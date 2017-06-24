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

#include <functional>
#include <memory>
#include <exception>
#include <mutex>

#include <boost/optional.hpp>

namespace ceema {

/**
 * Wraps a callable in shared_ptr so it can be copied
 * Used by lambdas as future callbacks
 */
    template<typename F>
    struct shared_function {
        std::shared_ptr<F> m_fptr;

        shared_function() = delete;

        shared_function(F &&f) : m_fptr(std::make_shared<F>(std::move(f))) {}

        shared_function(shared_function const &) = default;

        shared_function(shared_function &&) = default;

        template<typename...As>
        auto operator()(As &&...as) const {
            return (*m_fptr)(std::forward<As>(as)...);
        }
    };

    template<typename F>
    shared_function<std::decay_t<F> > make_shared_function(F &&f) {
        return {std::forward<F>(f)};
    }

    template<typename T>
    class future;

// Async data container
    template<typename T>
    class async_data : public std::enable_shared_from_this<async_data<T>> {
        std::mutex m_lock;
        boost::optional<T> m_data;
        std::exception_ptr m_exc;
        std::function<void(std::shared_ptr<async_data<T>>)> m_callback;

    public:
        async_data() {
            m_lock.lock();
        }

        T get() {
            m_lock.lock(); // No need to unlock until reset
            if (m_exc) {
                std::rethrow_exception(m_exc);
            }
            T res(std::move(*m_data));
            m_data.reset();
            return res;
        }

        void set_value(T t) {
            m_data = std::move(t);
            m_lock.unlock();
            if (m_callback) {
                m_callback(this->shared_from_this());
            }
        }

        void set_exception(std::exception_ptr exc) {
            m_exc = exc;
            m_lock.unlock();
            if (m_callback) {
                m_callback(this->shared_from_this());
            }
        }

        void set_callback(std::function<void(std::shared_ptr < async_data<T>>)> callback) {
            m_callback = std::move(callback);
            if (m_exc || m_data) {
                m_callback(this->shared_from_this());
            }
        }
    };

    template<>
    class async_data<void> : public std::enable_shared_from_this<async_data<void>> {
        std::mutex m_lock;
        std::exception_ptr m_exc;
        std::function<void(std::shared_ptr<async_data<void>>)> m_callback;
        bool m_data;

    public:
        async_data() : m_data(false) {
            m_lock.lock();
        }

        void get() {
            m_lock.lock(); // No need to unlock until reset
            if (m_exc) {
                std::rethrow_exception(m_exc);
            }
            m_data = false;
        }

        void set_value() {
            m_lock.unlock();
            m_data = true;
            if (m_callback) {
                m_callback(this->shared_from_this());
            }
        }

        void set_exception(std::exception_ptr exc) {
            m_exc = exc;
            m_lock.unlock();
            if (m_callback) {
                m_callback(this->shared_from_this());
            }
        }

        void set_callback(std::function<void(std::shared_ptr < async_data<void>>)> callback) {
            m_callback = std::move(callback);
            if (m_exc || m_data) {
                m_callback(this->shared_from_this());
            }
        }
    };

    template<typename T>
    using async_data_ptr = std::shared_ptr<async_data<T> >;


    /**
     * Read handle of an async data object
     * @tparam T
     */
    template<typename T>
    class future_base {
    public:
        async_data_ptr<T> m_data;

    public:
        //future_base() = default;

        future_base(async_data_ptr<T> d) : m_data(d) {}

        future_base(future_base const &) = delete;

        future_base(future_base &&) = default;

        future_base &operator=(future_base const &) = delete;

        future_base &operator=(future_base &&) = default;

        template<typename U = T>
        std::enable_if_t<!std::is_void<U>::value, T>
        get() {
            if (!m_data) {
                throw std::runtime_error("Invalid future");
            }
            T res = m_data->get();
            m_data.reset();
            return res;
        }

        template<typename U = T>
        std::enable_if_t<std::is_void<U>::value, T>
        get() {
            if (!m_data) {
                throw std::runtime_error("Invalid future");
            }
        }

        bool valid() const {
            return m_data.get();
        }

        explicit operator bool() const {
            return valid();
        }
    };

    /**
     * Read handle of an async data object
     * @tparam T
     */
    template<typename T>
    class future : public future_base<T> {
    public:
        using future_base<T>::future_base;

        explicit future() : future_base<T>(async_data_ptr<T>()) {}

        // Return a new future waiting for a callback
        template<typename F>
        inline auto next(F &&fun) -> future<decltype(fun((std::declval<future<T>>())))>;
    };

    /**
     * Read handle of an async data object
     * Specialized for wrapped future, which next() unwraps
     * @tparam U
     */
    template<typename U>
    class future<future<U> > : public future_base<future<U> > {
    public:
        typedef future<U> T;

        using future_base<T>::future_base;

        // Return a new future waiting for a callback
        template<typename F>
        inline auto next(F &&fun) -> future<decltype(fun((std::declval<future<U>>())))>;
    };

    // Write handle of an async data object
    template<typename T>
    class promise {
    private:
        async_data_ptr<T> data;
        future<T> fut;
    public:
        promise() : data(std::make_shared<async_data<T>>()), fut(data) {}

        promise(promise const &) = delete;

        promise(promise &&) = default;

        ~promise() {
            if (data) {
                data->set_exception(std::make_exception_ptr(
                        std::runtime_error("Broken promise")));
            }
        }

        promise &operator=(promise const &) = delete;

        promise &operator=(promise &&) = default;

        future<T> get_future() {
            if (!fut.valid()) {
                throw std::logic_error("future already taken");
            }
            return std::move(fut);
        }

        template<typename U = T>
        std::enable_if_t<!std::is_void<U>::value && std::is_copy_constructible<U>::value>
        set_value(U const &t) {
            if (!data) {
                throw std::runtime_error("Promise already satisfied");
            }
            data->set_value(t);
            data.reset();
        }

        template<typename U = T>
        std::enable_if_t<!std::is_void<U>::value && std::is_move_constructible<U>::value>
        set_value(U &&t) {
            if (!data) {
                throw std::runtime_error("Promise already satisfied");
            }
            data->set_value(std::move(t));
            data.reset();
        }

        template<typename U = T>
        std::enable_if_t<std::is_void<U>::value>
        set_value() {
            if (!data) {
                throw std::runtime_error("Promise already satisfied");
            }
            data->set_value();
            data.reset();
        }

        void set_exception(std::exception_ptr exc) {
            if (!data) {
                throw std::runtime_error("Promise already satisfied");
            }
            data->set_exception(exc);
            data.reset();
        }
    };


    template<typename F, typename R, typename T>
    void async_callback(F&& f, promise<R> p, async_data_ptr<T> dat) {
        // Re-create future to pass as callback argument
        try {
            // Call fun, useresult to set promise' value
            p.set_value(f(future<T>{dat}));
        } catch(std::exception& e) {
            p.set_exception(std::current_exception());
        }
    }

    template<typename F, typename T>
    void async_callback(F&& f, promise<void> p, async_data_ptr<T> dat) {
        // Re-create future to pass as callback argument
        try {
            // Call fun, useresult to set promise' value
            f(future<T>{dat});
            p.set_value();
        } catch(std::exception& e) {
            p.set_exception(std::current_exception());
        }
    }

    template<typename T>
    template<typename F>
    auto future<T>::next(F&& fun) -> future<decltype(fun((std::declval<future<T>>())))> {
        if (!this->valid()) {
            throw std::logic_error("Future has no data");
        }
        using R = decltype(fun(std::declval<future<T>>()));
        // Promise resulting from setting the callback
        promise<R> p;
        // Future that is returned
        future<R> fut_r = p.get_future();
        // Lambda captures the promise, and set the value upon call
        auto type_erase = [prom{std::move(p)}, f{std::move(fun)}](async_data_ptr<T>
        dat) mutable -> void {
                async_callback(std::move(f), std::move(prom), dat);
        };
        // Enable callback
        this->m_data->set_callback(make_shared_function(std::move(type_erase)));
        // invalidate self
        this->m_data = nullptr;
        // return the future associated with type erasing callback
        return std::move(fut_r);
    }

    template<typename U>
    template<typename F>
    auto future<future<U> >::next(F&& fun) -> future<decltype(fun((std::declval<future<U>>())))> {
        using R = decltype(fun(std::declval<future<U>>()));
        // Promise resulting from setting the callback
        promise<R> p;
        // Future that is returned
        future<R> fut_r = p.get_future();
        // Lambda captures the promise, and passing it one deeper upon call
        auto type_erase = [prom = std::move(p), f = std::move(fun)](async_data_ptr<T> dat) mutable -> void {
                // prom and f are now two local variables
                auto unwrapped_call =[prom=std::move(prom), f=std::move(f)](async_data_ptr<U> dat) mutable -> void {
                    async_callback(std::move(f), std::move(prom), dat);
                };
                try {
                    future<U> ft = dat->get();
                    ft.m_data->set_callback(make_shared_function(std::move(unwrapped_call)));
                } catch (std::exception &e) {
                    promise<U> p;
                    future<U> ft = p.get_future();
                    ft.m_data->set_callback(make_shared_function(std::move(unwrapped_call)));
                    p.set_exception(std::current_exception());
                }
        };
        // Enable callback
        this->m_data->set_callback(make_shared_function(std::move(type_erase)));
        // invalidate self
        this->m_data = nullptr;
        // return the future associated with type erasing callback
        return std::move(fut_r);
    }

}