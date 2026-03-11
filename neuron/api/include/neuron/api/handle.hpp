//
// Created by andy on 3/8/26.
//

#pragma once
#include <atomic>
#include <concepts>

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

        friend std::size_t hash_value(const Handle &obj) {
            std::size_t seed = 0x6DABBAEE;
            seed ^= (seed << 6) + (seed >> 2) + 0x60DC4FC4 + static_cast<std::size_t>(obj.pointer);
            return seed;
        }

        friend void swap(Handle &lhs, Handle &rhs) noexcept {
            using std::swap;
            swap(lhs.pointer, rhs.pointer);
        }

        template <class S>
            requires(std::derived_from<T, S>)
        inline operator Handle<S>() const {
            return Handle<S>(reinterpret_cast<S*>(pointer));
        }

        template <std::derived_from<T> D>
        explicit inline operator Handle<D>() const {
            return Handle(dynamic_cast<D *>(pointer));
        }

        template<typename S>
        inline operator Handle<S>() const {
            return Handle<S>(nullptr);
        }

        inline T *get() noexcept { return pointer; }

        inline const T *get() const noexcept { return pointer; }
    };
} // namespace neuron

namespace std {

    template <typename T>
    struct hash<neuron::Handle<T>> {
        inline auto operator()(const neuron::Handle<T> &obj) { return hash_value(obj); }
    };
} // namespace std
