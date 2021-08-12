#include <stdexcept>
#include <iostream>

#include "util_string.hh"

std::string split_last(std::string const &s, std::string const split_at)
{
    auto pos = s.find_last_of(split_at);
    if (pos == std::string::npos)
        return "";

    return s.substr(pos + 1);
}

std::vector<std::string> split_string(std::string s, std::string delimiter)
{
    std::vector<std::string> ans;

    size_t last = 0; 
    size_t next = 0; 
    while ((next = s.find(delimiter, last)) != std::string::npos) {   
        ans.push_back(s.substr(last, next-last));
        last = next + 1; 
    } 
       
    ans.push_back( s.substr(last));

    return ans;  
}

std::string copy_and_replace_all_substrs(const std::string &source, const std::string &from, const std::string &to)
{
    std::string ans;
    ans.reserve(source.length());

    std::string::size_type lastPos = 0;
    std::string::size_type findPos;

    while(std::string::npos != (findPos = source.find(from, lastPos)))
    {
        ans.append(source, lastPos, findPos - lastPos);
        ans += to;
        lastPos = findPos + from.length();
    }

    ans += source.substr(lastPos);
    
    return ans;
}

/* Replaces any c-like escaped characters in a string with the proper values.
 * Eg. '\' 'a' => 7 (bell)
 *
 * Only hex and octal escaped numbers valid (not UTF escapes)
 */
void deescape_string(std::string &s)
{
    bool is_escaped = false;
    bool octal_escape = false;
    bool hex_escape = false;

    for (int i = 0; i < s.size(); i++) {
        char c = s[i];
        bool is_last = i + 1 == s.size();
        redo:
        if (is_escaped) {
            is_escaped = false;
            bool hit = true;

            switch (c) {
            case 'n':
                s.replace(i - 1, 2, "\n");
                break;
            case 'a':
                s.replace(i - 1, 2, "\a");
                break;
            case 'v':
                s.replace(i - 1, 2, "\v");
                break;
            case 't':
                s.replace(i - 1, 2, "\t");
                break;
            case 'r':
                s.replace(i - 1, 2, "\r");
                break;
            case '\\':
                s.replace(i - 1, 2, "\\");
                break;
            case '\'':
                s.replace(i - 1, 2, "\'");
                break;
            case '\"':
                s.replace(i - 1, 2, "\"");
                break;
            case '\b':
                s.replace(i - 1, 2, "\b");
                break;
            case '\?':
                s.replace(i - 1, 2, "\?");
                break;
            default:
                hit = false;
            }

            if (hit) {
                i--;
                continue;
            }

            if (std::isdigit(c)) { /* Octal escape sequence. */
                octal_escape = true;
                goto redo;
            }

            if (c == 'x') {
                hex_escape = true;
                if (is_last)
                    throw std::runtime_error(
                            "Not valid escape in string literal: " + s);
                continue;
            }

            throw std::runtime_error(
                    "Not valid escape in string literal: " + s);

        } else if (octal_escape) {
            /* s[i] is a digit here */
            /* At most 3 characters. */
            for (int j = i; j < s.size() && j < i + 3; j++) {
                char c = s[j];
                bool is_last = j + 1 == s.size() || j == i + 2;
                if (!std::isdigit(c) || is_last) {
                    int n;
                    if (is_last && !std::isdigit(c))
                        n = j - i;
                    else
                        n = j - i + 1;
                    // s[n - 1] == '\\'
                    unsigned int oct;
                    try {
                        oct = std::stoul(s.substr(i, n), 0, 8);
                    } catch (std::invalid_argument &e) {
                        throw std::runtime_error(
                                "Not valid octal escape in string literal: "
                                        + s);
                    }
                    if (oct > 255)
                        throw std::runtime_error(
                                "Not valid octal escape in string literal: "
                                        + s);
                    s.erase(i - 1, n);
                    s[i - 1] = oct;
                    i--;
                    octal_escape = false;
                    break;
                }
            }
        } else if (hex_escape) {
            /* s[i] is a the first digit here */
            /* At most 2 characters. */
            std::cout << "i : " << i << std::endl;
            for (int j = i; (j < s.size()) && (j < i + 2); j++) {
                std::cout << "inner i : " << i << " j " << j << std::endl;
                char c = s[j];
                bool is_last = j + 1 == s.size() || j == i + 1;
                bool is_hex = std::isdigit(c) || (c <= 'F' && c >= 'A')
                        || (c <= 'f' && c >= 'a');
                if (!is_hex || is_last) {
                    int n;
                    if (is_last && !is_hex)
                        n = j - i;
                    else
                        n = j - i + 1;
                    // s[n - 1] == '\\'
                    unsigned int hex;
                    try {
                        std::cout << "Hex: " << s.substr(i, n) << std::endl;
                        hex = std::stoul(s.substr(i, n), 0, 16);
                    } catch (std::invalid_argument &e) {
                        throw std::runtime_error(
                                "Not valid hex escape in string literal: " + s);
                    }
                    if (hex > 255)
                        throw std::runtime_error(
                                "Not valid hex escape in string literal: " + s);
                    s.erase(i - 2, n + 1);
                    s[i - 2] = hex;
                    i -= 2;
                    hex_escape = false;
                    break;
                }
            }
        } else if (c == '\\') {
            is_escaped = true;
        }
    }
}