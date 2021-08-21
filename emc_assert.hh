#pragma once

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <numeric>
#include <type_traits>


#define THROW_USER_ERROR_LOC(msg)\
do {\
    std::cerr << __FILE__ << ":" << __LINE__ << " in " << __PRETTY_FUNCTION__ <<\
        "\n" << "Error in " << loc.first_line << ":" << loc.first_column << " to " <<\
        loc.last_line << ":" << loc.last_column << "\n" <<\
         msg <<\
        "\nAborting ..."<< std::endl;\
    throw std::runtime_error("Bad user code");\
} while((0))

#define THROW_USER_ERROR_WITH_LOC(msg, loc)\
do {\
    std::cerr << __FILE__ << ":" << __LINE__ << " in " << __PRETTY_FUNCTION__ <<\
        "\n" << "Error in " << loc.first_line << ":" << loc.first_column << " to " <<\
        loc.last_line << ":" << loc.last_column << "\n" <<\
         msg <<\
        "\nAborting ..."<< std::endl;\
    throw std::runtime_error("Bad user code");\
} while((0))

#define THROW_USER_ERROR(msg)\
do {\
    std::cerr << __FILE__ << ":" << __LINE__ << " in " << __PRETTY_FUNCTION__ <<\
        "\n" << "Error: " << "\n" << msg <<\
        "\nAborting ..."<< std::endl;\
    throw std::runtime_error("Bad user code");\
} while((0))

template<class T>
std::string type_to_string()
{
    return "";
}
template<int64_t>
std::string type_to_string()
{
    return "Long";
}
template<int32_t>
std::string type_to_string()
{
    return "Int";
}
template<int16_t>
std::string type_to_string()
{
    return "Short";
}
template<int8_t>
std::string type_to_string()
{
    return "Sbyte";
}
template<uint64_t>
std::string type_to_string()
{
    return "Ulong";
}
template<uint32_t>
std::string type_to_string()
{
    return "Uint";
}
template<uint16_t>
std::string type_to_string()
{
    return "Ushort";
}
template<uint8_t>
std::string type_to_string()
{
    return "Byte";
}
template<double>
std::string type_to_string()
{
    return "Double";
}
template<float>
std::string type_to_string()
{
    return "Float";
}


template<class V, class T>
void check_in_range(V v) 
{
    if (v > std::numeric_limits<T>::max())
        THROW_USER_ERROR("Variable with value: " + std::to_string(v) + 
        "too big for casting to a " + type_to_string<T>());
    if (v < std::numeric_limits<T>::lowest())
        THROW_USER_ERROR("Variable with value: " + std::to_string(v) + 
        "too small for casting to a " + type_to_string<T>());
}

template<uint64_t, class T>
void check_in_range(uint64_t v)
{
    if (v > std::numeric_limits<T>::max())
        THROW_USER_ERROR("Variable with value: " + std::to_string(v) + 
        "too big for casting to a " + type_to_string<T>());
}

template<class T>
void check_exact_as_double(T v)
{
}
template<uint64_t>
void check_exact_as_double(uint64_t v)
{
    double val = (double)v;
    uint64_t back = (uint64_t)val;
    if (back != v)
        THROW_USER_ERROR("Variable with type " + type_to_string<uint64_t>() + 
        "with value: " + std::to_string(v) + 
        "can't be represented exactly as a double.");
}
template<int64_t>
void check_exact_as_double(int64_t v)
{
    double val = (double)v;
    int64_t back = (int64_t)val;
    if (back != v)
        THROW_USER_ERROR("Variable with type " + type_to_string<int64_t>() + 
        "with value: " + std::to_string(v) + 
        "can't be represented exactly as a double.");
}

template<class T>
void check_exact_as_float(T v)
{
    float val = (float)v;
    T back = (T)val;
    if (back != v)
        THROW_USER_ERROR("Variable with type " + type_to_string<T>() + 
        "with value: " + std::to_string(v) + 
        "can't be represented exactly as a float.");
}
template<uint16_t>
void check_exact_as_float(uint16_t v) {}
template<int16_t>
void check_exact_as_float(int16_t v) {}
template<uint8_t>
void check_exact_as_float(uint8_t v) {}
template<int8_t>
void check_exact_as_float(int8_t v) {}

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