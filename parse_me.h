#include <stdlib.h>
struct struct_a {
    int a;
    int b;
};

int int_func(int i);

int struct_a_func(struct struct_a *a);

typedef struct struct_a struct_a_t;

extern int i;
extern struct_a_t s;

int foo() { return 1;}