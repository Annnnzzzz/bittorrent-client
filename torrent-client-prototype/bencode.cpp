#include "bencode.h"

namespace bencode {
    void get_dict(std::string& buffer, size_t& i);

    std::string get_string(std::string& buffer, size_t& i) {
        size_t start = i;
        while (buffer[i] != ':') {
            i++;
        }
        size_t end = i;
        std::string s = buffer.substr(start, end - start);
        int n = stoll(s);
        start = i + 1;
        std::string key = buffer.substr(start, n);
        i = start + n;
        return key;
    }

    size_t get_num(std::string& buffer, size_t& i) {
        i++;
        size_t start = i;
        while (buffer[i] != 'e') {
            i++;
        }
        std::string num = buffer.substr(start, i - start);
        size_t n = stoll(num);
        i++;
        return n;
    }

    void get_list(std::string& buffer, size_t& i) {
        i++;
        while (buffer[i] != 'e') {
            if (buffer[i] == 'i') {
                size_t n = get_num(buffer, i);
            }
            else if (buffer[i] == 'l') {
                get_list(buffer, i);
            }
            else if (buffer[i] == 'd') {
                get_dict(buffer, i);
            }
            else {
                std::string f = get_string(buffer, i);
            }
        }
        i++;
    }

    void get_dict(std::string& buffer, size_t& i) {
        i++;
        while (buffer[i] != 'e') {
            if (buffer[i] == 'i') {
                size_t n = get_num(buffer, i);
            }
            else if (buffer[i] == 'l') {
                get_list(buffer, i);
            }
            else if (buffer[i] == 'd') {
                get_dict(buffer, i);
            }
            else {
                std::string f = get_string(buffer, i);
            }
        }
        i++;
    }
}
