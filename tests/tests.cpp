#include <poly_value/poly_value.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("check empty methods", "[poly_value]") {
    struct A {
        virtual ~A() {}
    };

    auto pv = pv::poly_value<A, 8>();

    CHECK(!pv);
    CHECK(!pv.has_value());

    pv.emplace<A>();

    CHECK(pv);
    CHECK(pv.has_value());

    pv.reset();

    CHECK(!pv);
    CHECK(!pv.has_value());
}

TEST_CASE("test copy", "[poly_value]") {
    struct A {
        virtual ~A() {}
    };

    SECTION("Copyable") {
        using pv = pv::poly_value<A, 16, pv::flags::Copyable>;
        pv value(A{});
        pv newval;

        SECTION("move") {
            newval = std::move(value);
            CHECK(value.has_value());
            CHECK(newval.has_value());

            value = std::move(newval);
            CHECK(value.has_value());
            CHECK(newval.has_value());
        }

        SECTION("copy") {
            newval = value;
            CHECK(value.has_value());
            CHECK(newval.has_value());

            value = newval;
            CHECK(value.has_value());
            CHECK(newval.has_value());
        }
    }

    SECTION("Moveable") {
        using pv = pv::poly_value<A, 16, pv::flags::Moveable>;
        pv value(A{});
        pv newval;

        SECTION("move") {
            newval = std::move(value);

            CHECK_FALSE(value.has_value());
            CHECK(newval.has_value());

            value = std::move(newval);

            CHECK(value.has_value());
            CHECK_FALSE(newval.has_value());
        }

        // SECTION("copy") {
        // newval = value; compile error
        // }
    }

    SECTION("Copyable and Moveable") {
        using pv = pv::poly_value<A, 16, pv::flags::Moveable | pv::flags::Copyable>;

        pv value(A{});
        pv newval;

        SECTION("move") {
            newval = std::move(value);

            CHECK_FALSE(value.has_value());
            CHECK(newval.has_value());

            value = std::move(newval);

            CHECK(value.has_value());
            CHECK_FALSE(newval.has_value());
        }

        SECTION("copy") {
            newval = value;
            CHECK(value.has_value());
            CHECK(newval.has_value());

            value = newval;
            CHECK(value.has_value());
            CHECK(newval.has_value());
        }
    }
}

TEST_CASE("destructor calls", "[poly_value]") {
    struct A {
        virtual ~A() {
            dtor_was_called = true;
        }
        A(bool& dtor_was_called): dtor_was_called(dtor_was_called) {}
        bool& dtor_was_called;
    };

    auto pv = pv::poly_value<A, 16>();

    bool dtor_was_called = false;
    pv.emplace<A>(dtor_was_called);

    CHECK(pv.has_value());
    CHECK_FALSE(dtor_was_called);

    pv.reset();

    CHECK_FALSE(pv.has_value());
    CHECK(dtor_was_called);
}

TEST_CASE("Assign different values", "[poly_value]") {
    struct A {
        virtual ~A() = default;
        virtual int apply(int in) = 0;
    };

    struct B: public A {
        int v;
        B(int v): v(v) {}

        int apply(int in) override {
            return in + v;
        }
    };

    struct C: public A {
        int v;
        C(int v): v(v) {}

        int apply(int in) override {
            return in - v;
        }
    };

    auto value = pv::poly_value<A, 16>();
    value.emplace<B>(10);
    int i = 17;
    CHECK(value->apply(i) == 27);

    pv::poly_value<A, 16> newval;

    newval.emplace<C>(10);
    value = newval;

    CHECK(value->apply(i) == 7);
}
