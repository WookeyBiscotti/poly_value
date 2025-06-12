#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace pv {

namespace details {

template<bool C>
class poly_value_base_copy {
public:
    poly_value_base_copy() noexcept = default;
    poly_value_base_copy(const poly_value_base_copy&) = delete;
    poly_value_base_copy(poly_value_base_copy&&) = delete;
    poly_value_base_copy& operator=(const poly_value_base_copy&) = delete;
    poly_value_base_copy& operator=(poly_value_base_copy&&) = delete;

    using CopyFn = void (*)(const void* from, void* to);

    template<class D>
    void generate_copy_fn() noexcept {}
    CopyFn copy_fn() const noexcept;
    void set_copy_fn(CopyFn) noexcept;
};

template<>
class poly_value_base_copy<true> {
public:
    poly_value_base_copy() noexcept = default;
    poly_value_base_copy(const poly_value_base_copy&) = delete;
    poly_value_base_copy(poly_value_base_copy&&) = delete;
    poly_value_base_copy& operator=(const poly_value_base_copy&) = delete;
    poly_value_base_copy& operator=(poly_value_base_copy&&) = delete;

    template<class D>
    void generate_copy_fn() noexcept {
        static_assert(
            std::is_copy_constructible_v<D> || (std::is_default_constructible_v<D> && std::is_copy_assignable_v<D>));

        if constexpr (std::is_copy_constructible_v<D>) {
            _copy = [](const void* from, void* to) noexcept(
                        std::is_nothrow_copy_constructible_v<D>) { new (to)(D)(*static_cast<const D*>(from)); };
        } else {
            _copy = [](const void* from, void* to) noexcept(
                        std::is_nothrow_default_constructible_v<D>&& std::is_nothrow_copy_assignable_v<D>) {
                auto v = new (to)(D)();
                v = *static_cast<const D*>(from);
            };
        }
    }

    using CopyFn = void (*)(const void* from, void* to);
    CopyFn copy_fn() const noexcept {
        return _copy;
    }
    void set_copy_fn(CopyFn fn) {
        _copy = fn;
    }

private:
    CopyFn _copy = nullptr;
};

template<bool M>
class poly_value_base_move {
public:
    poly_value_base_move() noexcept = default;
    poly_value_base_move(const poly_value_base_move&) = delete;
    poly_value_base_move(poly_value_base_move&&) = delete;
    poly_value_base_move& operator=(const poly_value_base_move&) = delete;
    poly_value_base_move& operator=(poly_value_base_move&&) = delete;

    using MoveFn = void (*)(void* from, void* to);

    template<class D>
    constexpr void generate_move_fn() noexcept {}
    MoveFn move_fn() const noexcept;
    void set_move_fn(MoveFn fn) noexcept {};
};

template<>
class poly_value_base_move<true> {
public:
    poly_value_base_move() noexcept = default;
    poly_value_base_move(const poly_value_base_move&) = delete;
    poly_value_base_move(poly_value_base_move&&) = delete;
    poly_value_base_move& operator=(const poly_value_base_move&) = delete;
    poly_value_base_move& operator=(poly_value_base_move&&) = delete;

    using MoveFn = void (*)(void* from, void* to);

    template<class D>
    void generate_move_fn() noexcept {
        static_assert(
            std::is_move_constructible_v<D> || (std::is_default_constructible_v<D> && std::is_move_assignable_v<D>));

        if constexpr (std::is_move_constructible_v<D>) {
            _move = [](void* from, void* to) noexcept(
                        std::is_nothrow_move_constructible_v<D>) { new (to)(D)(std::move(*static_cast<D*>(from))); };
        } else {
            _move = [](void* from, void* to) noexcept(
                        std::is_nothrow_default_constructible_v<D>&& std::is_nothrow_move_assignable_v<D>) {
                auto v = new (to)(D)();
                v = std::move(*static_cast<D*>(from));
            };
        }
    }
    MoveFn move_fn() const noexcept {
        return _move;
    }
    void set_move_fn(MoveFn fn) noexcept {
        _move = fn;
    }

private:
    MoveFn _move = nullptr;
};
} // namespace details

struct flags {
    enum bits : std::uint64_t {
        Empty = 0,
        Copyable = 1 << 0,
        Moveable = 1 << 1,
        NoexceptCopy = 1 << 3,
        NoexceptMove = 1 << 4
    };
};

template<class Base, std::size_t Size, std::uint64_t Flags = flags::Copyable | flags::Moveable>
class poly_value {
    static constexpr bool Copyable = Flags & flags::Copyable;
    static constexpr bool Moveable = Flags & flags::Moveable;
    static constexpr bool NoexceptCopy = Flags & flags::NoexceptCopy;
    static constexpr bool NoexceptMove = Flags & flags::NoexceptMove;

public:
    template<class D, class... Args>
    static poly_value make(Args&&... args) noexcept(std::is_nothrow_constructible_v<D, Args...>) {
        poly_value pv;
        pv.emplace<D>(std::forward<Args>(args)...);

        return pv;
    }

    template<class D>
    poly_value(const D& value) noexcept(NoexceptCopy)
        requires(std::is_base_of_v<Base, D> && std::is_copy_constructible_v<D>)
    {
        emplace<D>(value);
    }

    template<class D>
    poly_value(D&& value) noexcept(NoexceptMove)
        requires(std::is_base_of_v<Base, D> && std::is_move_constructible_v<D>)
    {
        emplace<D>(std::move(value));
    }

    poly_value() noexcept = default;
    poly_value(std::nullptr_t) noexcept {}

    poly_value(const poly_value& other) noexcept(NoexceptCopy) {
        operator=(other);
    }
    auto& operator=(const poly_value& other) noexcept(NoexceptCopy)
        requires(Copyable)
    {
        destroy();
        if (other.has_value()) {
            set_base_ptr_offset(other.base_ptr_offset());

            _copy.set_copy_fn(other._copy.copy_fn());
            if constexpr (Moveable) {
                _move.set_move_fn(other._move.move_fn());
            }
            other._copy.copy_fn()(other._storage, this->_storage);
        }
        return *this;
    }
    template<class D>
    auto& operator=(const D& other) noexcept(NoexceptCopy)
        requires(Copyable && std::is_base_of_v<Base, D>)
    {
        emplace<D>(other);

        return *this;
    }

    poly_value(poly_value&& other) noexcept(NoexceptMove): poly_value() {
        operator=(std::move(other));
    }
    auto& operator=(poly_value&& other) noexcept(NoexceptMove)
        requires(Moveable)
    {
        destroy();
        if (other.has_value()) {
            set_base_ptr_offset(other.base_ptr_offset());

            _move.set_move_fn(other._move.move_fn());
            if constexpr (Copyable) {
                _copy.set_copy_fn(other._copy.copy_fn());
            }

            other._move.move_fn()(other._storage, this->_storage);

            other.destroy_and_clear();
        }
        return *this;
    }

    template<class D>
    auto& operator=(D&& other) noexcept(NoexceptMove)
        requires(Moveable && std::is_base_of_v<Base, D>)
    {
        emplace<D>(std::move(other));

        return *this;
    }

    ~poly_value() {
        destroy();
    }

    template<class D, class... Args>
    void emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<D, Args...>) {
        static_assert(std::is_base_of_v<Base, D>);
        static_assert(sizeof(D) <= Size);

        destroy();

        _base_ptr = new (_storage) D(std::forward<Args>(args)...);

        _copy.template generate_copy_fn<D>();
        _move.template generate_move_fn<D>();
    }

    void emplace(const poly_value& other) noexcept(NoexceptCopy) {
        this *= other;
    }
    void emplace(poly_value&& other) noexcept(NoexceptMove) {
        this *= std::move(other);
    }

    Base* get() noexcept {
        return get_base();
    }
    const Base* get() const noexcept {
        return get_base();
    }
    Base* operator->() noexcept {
        return get_base();
    }
    const Base* operator->() const noexcept {
        return get_base();
    }
    Base* operator*() noexcept {
        return get_base();
    }
    const Base* operator*() const noexcept {
        return get_base();
    }

    bool has_value() const noexcept {
        return _base_ptr;
    }

    operator bool() const noexcept {
        return has_value();
    }

    void reset() {
        destroy_and_clear();
    }

private:
    Base* get_base() {
        return _base_ptr;
    }
    const Base* get_base() const {
        return _base_ptr;
    }

    void destroy_and_clear() {
        destroy();

        _base_ptr = nullptr;
    }

    void destroy() {
        if (has_value()) {
            get_base()->~Base();
        }
    }

    std::size_t base_ptr_offset() const noexcept {
        return reinterpret_cast<const std::byte*>(_base_ptr) - _storage;
    }

    void set_base_ptr_offset(std::size_t new_offset) noexcept {
        _base_ptr = reinterpret_cast<Base*>(_storage + new_offset);
    }

private:
    Base* _base_ptr = nullptr;

    details::poly_value_base_copy<Copyable> _copy;
    details::poly_value_base_move<Moveable> _move;

    alignas(Base) std::byte _storage[Size];
};

} // namespace pv
