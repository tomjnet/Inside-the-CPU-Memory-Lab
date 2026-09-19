/* Stack vs Heap: Where Does Your Data Actually Live? - slide 11: std::unique_ptr: the owner lives on the stack (C version of 11_unique_ptr.cpp) */
/* Build: make 11_unique_ptr_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

#include <stdio.h>
#include <stdlib.h>

/* C has no destructor and no std::unique_ptr: the owner is a one pointer struct on the stack,
   and order_owner_reset is the delete the compiler would have written for us. */
typedef struct { int qty; double price; } Order;
typedef struct { Order *ptr; } OrderOwner;

static OrderOwner order_make(int q, double p) {
    OrderOwner o = { malloc(sizeof(Order)) };
    if (o.ptr != NULL) {
        o.ptr->qty = q;
        o.ptr->price = p;
        printf("  Order constructed on the heap at %p\n", (void *)o.ptr);
    }
    return o;
}

static void order_owner_reset(OrderOwner *o) {   /* the destructor, called by hand */
    if (o->ptr != NULL) {
        free(o->ptr);
        o->ptr = NULL;
        printf("  Order destroyed: free ran, and we had to write it\n");
    }
}

static OrderOwner order_move(OrderOwner *from) { /* ownership moves, no copy */
    OrderOwner to = { from->ptr };
    from->ptr = NULL;
    return to;
}

int main(void) {
    printf("entering the scope\n");
    {
        OrderOwner p = order_make(7, 101.5);   /* one malloc inside */
        if (p.ptr == NULL) return 1;
        p.ptr->qty += 1;                       /* used like a raw pointer */
        printf("  p lives on the stack at %p and owns %p (qty %d, price %g)\n",
               (void *)&p, (void *)p.ptr, p.ptr->qty, p.ptr->price);
        OrderOwner q = order_move(&p);
        /* p.ptr is NULL now, q owns the Order */
        printf("  after order_move: p is %s, q owns %p\n",
               p.ptr == NULL ? "NULL" : "not null", (void *)q.ptr);
        printf("  reaching the closing brace\n");
        order_owner_reset(&q);                 /* q leaves scope: in C we free by hand */
        order_owner_reset(&p);                 /* already NULL: does nothing, no double free */
    }
    printf("left the scope\n\n");
    /* sizeof(OrderOwner) == sizeof(Order *): 8 B, no overhead */
    _Static_assert(sizeof(OrderOwner) == sizeof(Order *), "the owner is one pointer");
    printf("sizeof(OrderOwner) = %zu B, sizeof(Order *) = %zu B\n", sizeof(OrderOwner), sizeof(Order *));
    /* OrderOwner copy = q;   compiles in C: nothing stops two owners, so the move function is a convention */
    return 0;
}
