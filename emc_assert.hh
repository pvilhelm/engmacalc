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
        throw std::runtime_error("Bug");\
    }\
} while((0))\

#define DEBUG_ASSERT_NOTNULL(var) DEBUG_ASSERT(var != nullptr , "Anticipates non-null " << #var)

#else
#define DEBUG_ASSERT(exp, msg)
#endif
