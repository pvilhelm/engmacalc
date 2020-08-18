/* C-linkage functions that are supposed to be call from within the JITed context. */

#include <iostream>

extern "C" void printnl_int(int i)
{
    std::cout << i << std::endl;
}

extern "C" void printnl_double(double d)
{
    std::cout << d << std::endl;
}
