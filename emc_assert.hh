#pragma once

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#define THROW_USER_ERROR_LOC(msg)\
do {\
    std::cerr << __FILE__ << ":" << __LINE__ << " in " << __PRETTY_FUNCTION__ <<\
        "\n" << "Error in " << loc.first_line << ":" << loc.first_column << " to " <<\
        loc.last_line << ":" << loc.last_column << "\n" <<\
         msg <<\
        "\nAborting ..."<< std::endl;\
    throw std::runtime_error("Bad user code");\
} while((0))

#define THROW_NOT_IMPLEMENTED(msg)\
do {\
    std::cerr << __FILE__ << ":" << __LINE__ << " in " << __PRETTY_FUNCTION__ <<\
        "\nEnded up in not implemented codepath:\n" << msg <<\
        "\nAborting ..."<< std::endl;\
    throw std::runtime_error("Not implemented");\
} while((0))

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