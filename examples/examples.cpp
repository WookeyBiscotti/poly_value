#include <poly_value/poly_value.hpp>

#include <iostream>

int main(int argc, const char** argv) {
    struct A {
        virtual ~A() = default;
        virtual void print() = 0;
    };

    struct B: A {
        void print() override {
            std::cout << "B::print " << value << std::endl;
        }
        int value = 42;
    };

    struct C: A {
        void print() override {
            std::cout << "C::print " << value << std::endl;
        }

        float value = 3.1415f;
    };

    using poly_value = pv::poly_value<A, 16>;

    poly_value pv1 = B{};
    poly_value pv2 = C{};

    pv1->print(); // `B::print 42`
    pv2->print(); // `C::print 3.1415`

    pv1 = pv2;
    pv1->print(); // `C::print 3.1415`

    pv2 = B{};
    pv1 = std::move(pv2); // pv2 value was moved
    pv1->print();         // `B::print 42`

    std::cout << pv2.has_value() << std::endl; // 0

    return 0;
}
