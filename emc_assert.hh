#pragma once

#include <cstdlib>
#include <iostream>
#include <stdexcept>

// THROW_NOT_IMPLEMENTED("");
#define THROW_NOT_IMPLEMENTED(msg)\
do {\
    std::cerr << __FILE__ << ":" << __LINE__ << " in " << __PRETTY_FUNCTION__ <<\
        "\nEnded up in not implemented codepath:\n" << msg <<\
        "\nAborting ..."<< std::endl;\
    throw std::runtime_error("Not implemented");\
} while((0))

// THROW_BUG("");
#define THROW_BUG(msg)\
do {\
    std::cerr << __FILE__ << ":" << __LINE__ << " in " << __PRETTY_FUNCTION__ <<\
        "\nEnded up in a bad state:\n" << msg <<\
        "\nAborting ..."<< std::endl;\
    throw std::runtime_error("Bad state");\
} while((0))

#define ASSERT(exp, msg)\
do {\
    if(!(exp)) {\
        std::cerr << __FILE__ << ":" << __LINE__ << " in " << __PRETTY_FUNCTION__ <<\
            "\nAssertion >>" << #exp << "<< failed ... with msg:\n" << msg <<\
            "\nAborting ..."<< std::endl;\
        throw std::runtime_error("Bug");\
    }\
} while((0))


#ifndef NDEBUG
#define DEBUG_ASSERT(exp, msg) ASSERT(exp, msg)
#define DEBUG_ASSERT_NOTNULL(var) DEBUG_ASSERT(var != nullptr , "Anticipates non-null " << #var)

#else
#define DEBUG_ASSERT(exp, msg)
#define DEBUG_ASSERT_NOTNULL(var)
#endif