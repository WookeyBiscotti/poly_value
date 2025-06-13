#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace pv {

namespace details {

template<bool Copyable, bool Moveable>
class poly_value_copy_move {
public:
    enum class Op {
        Copy,
        Move,
    };

    using CopyMoveFn = void (*)(Op op, void* destination, const void* source);

    poly_value_copy_move() noexcept = default;
    poly_value_copy_move(const poly_value_copy_move&) = default;
    poly_value_copy_move(poly_value_copy_move&&) = default;
    poly_value_copy_move& operator=(const poly_value_copy_move&) = default;
    poly_value_copy_move& operator=(poly_value_copy_move&&) = default;

    template<class D>
    void generate_copy_move_fn() noexcept {
        _fn = [](Op op, void* destination, const void* source) {
            switch (op) {
            case Op::Copy: {
                if constexpr (std::is_copy_constructible_v<D>) {
                    new (destination)(D)(*static_cast<const D*>(source));
                } else {
                    auto v = new (destination)(D)();
                    v = *static_cast<const D*>(source);
                }
                break;
            }
            case Op::Move: {
                if constexpr (std::is_move_constructible_v<D>) {
                    new (destination)(D)(std::move(*static_cast<D*>(const_cast<void*>(source))));
                } else {
                    auto v = new (destination)(D)();
                    v = std::move(*static_cast<D*>(const_cast<void*>(source)));
                }
                break;
            }
            }
        };
    }

    void copy(void* destination, const void* source) const {
        _fn(Op::Copy, destination, source);
    }

    void move(void* destination, const void* source) const {
        _fn(Op::Move, destination, source);
    }

private:
    CopyMoveFn _fn;
};

template<>
class poly_value_copy_move<false, false> {
public:
    poly_value_copy_move() noexcept = default;
    poly_value_copy_move(const poly_value_copy_move&) = delete;
    poly_value_copy_move(poly_value_copy_move&&) = delete;
    poly_value_copy_move& operator=(const poly_value_copy_move&) = delete;
    poly_value_copy_move& operator=(poly_value_copy_move&&) = delete;

    template<class D>
    void generate_copy_move_fn() noexcept {}
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
            _copy_move_fn = other._copy_move_fn;
            set_base_ptr_offset(other.base_ptr_offset());

            _copy_move_fn.copy(_storage, other._storage);
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
            _copy_move_fn = other._copy_move_fn;
            set_base_ptr_offset(other.base_ptr_offset());

            _copy_move_fn.move(_storage, other._storage);

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

        _copy_move_fn.template generate_copy_move_fn<D>();
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

    details::poly_value_copy_move<Copyable, Moveable> _copy_move_fn;

    alignas(Base) std::byte _storage[Size];
};

} // namespace pv
