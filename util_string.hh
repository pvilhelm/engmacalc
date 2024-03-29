
#include <string>
#include <vector>

bool ends_with(std::string const &s, std::string const &ending);
bool ends_with(std::string const &s, std::vector<std::string> const &ending);

std::string split_last(std::string const &s, std::string const split_at);
std::string strip_last(std::string const &s, std::string const split_at);

std::vector<std::string> split_string(std::string s, std::string delimiter);

std::string copy_and_replace_all_substrs(const std::string &source, const std::string &from, const std::string &to);

void deescape_string(std::string &s);