#pragma once

#include <cstdlib>
#include <iostream>

#ifndef NDEBUG
#define DEBUG_ASSERT(exp, msg)\
do {\
    if(!(exp)) {\
        std::cerr << __FILE__ << ":" << __LINE__ << " in " << __PRETTY_FUNCTION__ <<\
            "\nAssertion >>" << #exp << "<< failed ... with msg:\n" << msg <<\
            "\nAborting ..."<< std::endl;\
        exit(1);\
    }\
} while((0))\


#else
#define DEBUG_ASSERT(exp, msg)
#endif
