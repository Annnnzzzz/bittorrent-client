#include "torrent_file.h"
#include "bencode.h"
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <sstream>

int u = 0;
TorrentFile LoadTorrentFile(const std::string& filename) {
    u++;
    TorrentFile tf;
    std::ifstream file(filename, std::ios::binary);
    std::string line;
    std::stringstream buff;
    buff << file.rdbuf();
    file.close();
    std::string buffer = buff.str();
    size_t i = 0;
    i++;
    int f = 0;
    while (i < buffer.size()) {
        if (buffer[i] == 'e') {
            i++;
        }
        else {
            std::string key = bencode::get_string(buffer, i);
            if (key == "announce") {
                std::string url = bencode::get_string(buffer, i);
                tf.announce = url;
                f++;
                if (f == 3) {
                    return tf;
                }
            }
            else if (key == "announce-list") {
                i++; 
                while (buffer[i] != 'e') { 
                    if (buffer[i] == 'l') { 
                        i++;
                        while (buffer[i] != 'e') {
                            std::string url = bencode::get_string(buffer, i);
                            tf.announceList.push_back(url);
                        }
                        i++;
                    }
                }
                i++;
            }
            else if (key == "comment") {
                std::string comm = bencode::get_string(buffer, i);
                tf.comment = comm;
                f++;
                if (f == 3) {
                    return tf;
                }
               
            }
            else if (key == "info") {
                size_t b_st = i;
                i++;
                int m = 0;
                while (buffer[i] != 'e') {
                    std::string key_2 = bencode::get_string(buffer, i);
                    if (key_2 == "length") {
                        i++;
                        size_t start = i;
                        while (buffer[i] != 'e') {
                            i++;
                        }
                        std::string num = buffer.substr(start, i - start);

                        size_t n = stoll(num);
                        tf.length = n;
                        i++;
                        m++;
                        
                    }
                    else if (key_2 == "name") {
                        std::string nn = bencode::get_string(buffer, i);
                        tf.name = nn;
                        m++;
                    }
                    else if (key_2 == "piece length") {
                        i++;
                        size_t start = i;
                        while (buffer[i] != 'e') {
                            i++;
                        }
                        std::string num = buffer.substr(start, i - start);
                        size_t n = stoll(num);
                        tf.pieceLength = n;
                        i++;
                        m++;
                        
                    }
                    else if (key_2 == "pieces") {
                        size_t start = i;
                        while (buffer[i] != ':') {
                            i++;
                        }
                        std::string num = buffer.substr(start, i - start);
                        size_t count = stoll(num);
                        count = count / 20;
                        i++;
                        for (int j = 0; j < count; ++j) {
                            tf.pieceHashes.push_back(buffer.substr(i, 20));
                            i += 20;
                        }
                        m++;
                       
                    }
                    else {
                        if (buffer[i] == 'i') {
                            size_t n = bencode::get_num(buffer, i);
                        }
                        else if (buffer[i] == 'l') {
                            bencode::get_list(buffer, i);
                        }
                        else if (buffer[i] == 'd') {
                            bencode::get_dict(buffer, i);
                        }
                        else {
                            std::string f = bencode::get_string(buffer, i);
                        }
                    }
                    if (m == 4) {
                        break;
                    }
                }
                size_t b_end = i;
                std::string b = buffer.substr(b_st, i - b_st + 1);
                unsigned char hash[20];
                SHA1(reinterpret_cast<const unsigned char*>(b.data()), b.size(), hash);
                std::string result;
                for (size_t r = 0; r < 20; ++r) {
                    result += hash[r];
                }
                //cout << result << endl;
                tf.infoHash = result;
                f++;
                if (f == 3) {
                    return tf;
                }
            }
            else {
                if (buffer[i] == 'i') {
                    size_t n = bencode::get_num(buffer, i);
                }
                else if (buffer[i] == 'l') {
                    bencode::get_list(buffer, i);
                }
                else if (buffer[i] == 'd') {
                    bencode::get_dict(buffer, i);
                }
                else {
                    std::string f = bencode::get_string(buffer, i);
                }
            }
        }
    }
    return tf;
    // Перенесите сюда функцию загрузки файла из предыдущего домашнего задания
}




