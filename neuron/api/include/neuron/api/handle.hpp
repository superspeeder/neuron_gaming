//
// Created by andy on 3/8/26.
//

#pragma once
#include <atomic>

namespace neuron {
    template <typename T>
    class Handle {
        T *pointer;

      public:
        inline explicit Handle(T *p) : pointer(p) {}

        Handle(const Handle &other)                = default;
        Handle(Handle &&other) noexcept            = default;
        Handle &operator=(const Handle &other)     = default;
        Handle &operator=(Handle &&other) noexcept = default;

        friend bool operator==(const Handle &lhs, const Handle &rhs) { return lhs.pointer == rhs.pointer; }

        friend bool operator!=(const Handle &lhs, const Handle &rhs) { return !(lhs == rhs); }

        inline operator bool() const noexcept { return pointer != nullptr; }

        inline const T &operator*() const noexcept { return *pointer; }

        inline T &operator*() noexcept { return *pointer; }

        inline const T *operator->() const noexcept { return pointer; }

        inline T *operator->() noexcept { return pointer; }

        template <typename T2>
        inline static Handle<T2> of(T2 *p) {
            return Handle<T2>(p);
        }
    };
} // namespace neuron
