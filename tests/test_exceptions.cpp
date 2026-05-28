#include <iostream>
#include <stdexcept>
#include "ExceptionHandler.h"

// Test for std::bad_cast fix - using reference dynamic_cast which throws on failure
void TestBadCastReference() {
    struct Base { virtual ~Base() {} };
    struct Derived1 : Base {};
    struct Derived2 : Base {};

    Derived1* d1 = new Derived1();
    try {
        Base& bRef = *d1;
        Derived2& d2Ref = dynamic_cast<Derived2&>(bRef);
        (void)d2Ref;
        std::cerr << "ERROR: dynamic_cast reference did not throw" << std::endl;
    } catch (const std::bad_cast& e) {
        std::cout << "PASS: caught bad_cast: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "ERROR: caught unexpected exception type" << std::endl;
    }
    delete d1;
}

// Test for std::bad_typeid - using nullptr which throws
void TestBadTypeid() {
    struct Polymorphic { virtual ~Polymorphic() {} };
    Polymorphic* p = nullptr;
    try {
        (void)typeid(*p);
        std::cerr << "ERROR: typeid(*p) did not throw for nullptr" << std::endl;
    } catch (const std::bad_typeid& e) {
        std::cout << "PASS: caught bad_typeid: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "ERROR: caught unexpected exception type" << std::endl;
    }
}

int main() {
    ExceptionHandler::Init("/tmp/test_stacktrace.log");

    std::cout << "Running unit tests for exception type fixes..." << std::endl;

    TestBadCastReference();
    TestBadTypeid();

    std::cout << "All unit tests completed." << std::endl;
    return 0;
}