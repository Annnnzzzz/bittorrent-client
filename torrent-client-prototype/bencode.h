#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>
#include <algorithm>

namespace bencode {
    std::string get_string(std::string& buffer, size_t& i);

    size_t get_num(std::string& buffer, size_t& i);

    void get_list(std::string& buffer, size_t& i);

    void get_dict(std::string& buffer, size_t& i);
}
