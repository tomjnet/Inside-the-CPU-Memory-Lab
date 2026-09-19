// Stack vs Heap: Where Does Your Data Actually Live? - slide 11: std::unique_ptr: the owner lives on the stack
// Build: make 11_unique_ptr
#include <iostream>
#include <memory>
#include <utility>

struct Order {
    int qty;
    double price;
    Order(int q, double p) : qty(q), price(p) { std::cout << "  Order constructed on the heap at " << this << "\n"; }
    ~Order() { std::cout << "  Order destroyed: delete ran, nobody wrote it\n"; }
};

int main() {
    std::cout << "entering the scope\n";
    {
        auto p = std::make_unique<Order>(7, 101.5);  // one new inside
        p->qty += 1;                       // used like a raw pointer
        std::cout << "  p lives on the stack at " << &p << " and owns " << p.get()
                  << " (qty " << p->qty << ", price " << p->price << ")\n";   // added
        auto q = std::move(p);             // ownership moves, no copy
        // p is nullptr now, q owns the Order
        std::cout << "  after std::move: p is " << (p == nullptr ? "nullptr" : "not null")
                  << ", q owns " << q.get() << "\n";                          // added
        std::cout << "  reaching the closing brace\n";                        // added
    }                                      // q leaves scope: delete runs
    std::cout << "left the scope\n\n";
    // sizeof(std::unique_ptr<Order>) == sizeof(Order*): 8 B, no overhead
    static_assert(sizeof(std::unique_ptr<Order>) == sizeof(Order*), "unique_ptr is one pointer");
    std::cout << "sizeof(std::unique_ptr<Order>) = " << sizeof(std::unique_ptr<Order>)
              << " B, sizeof(Order*) = " << sizeof(Order*) << " B\n";
    // auto copy = q;   would not compile: a unique_ptr cannot be copied, two owners would mean two deletes
    return 0;
}
