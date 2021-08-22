
#include <stdio.h>

//FUNC Int i = c::exportedbacktoc_returns_int_one() DO
int exportedbacktoc_returns_int_one();
//FUNC Int i = c::exportedbacktoc_returns_int_plus_one(Int i) DO
int exportedbacktoc_returns_int_plus_one(int);
//FUNC Double d = c::exportedbacktoc_returns_doubles_summed(Double d1, Double d2) DO
double exportedbacktoc_returns_doubles_summed(double, double);
//FUNC Int i = c::exportedtoc_returns_int_one() DO
int exportedtoc_returns_int_one();
//FUNC Int i = c::exportedtoc_returns_int_plus_one(Int i) DO
int exportedtoc_returns_int_plus_one(int);
//FUNC Double d = c::exportedtoc_returns_doubles_summed(Double d1, Double d2) DO
double exportedtoc_returns_doubles_summed(double, double);


int main (int argc, char **argv)
{
    if (exportedbacktoc_returns_int_one() != 1)
        printf("FAIL");
    if (exportedbacktoc_returns_int_plus_one(2) != 3)
        printf("FAIL");
    if (exportedbacktoc_returns_doubles_summed(3.5, 4.5) != 8.)
        printf("FAIL");
    if (exportedtoc_returns_int_one() != 1)
        printf("FAIL");
    if (exportedtoc_returns_int_plus_one(2) != 3)
        printf("FAIL");
    if (exportedtoc_returns_doubles_summed(3.5, 4.5) != 8.)
        printf("FAIL");
    printf("DONE");
    return 0;
}