#include "torrent_tracker.h"
#include "bencode.h"
#include "byte_tools.h"
#include <cpr/cpr.h>
#include <iostream>

int v = 0;

void TorrentTracker::UpdatePeers(const TorrentFile& tf, std::string peerId, int port) {
    v++;
    cpr::Response res = cpr::Get(
        cpr::Url{ tf.announce },
        cpr::Parameters{
                {"info_hash", tf.infoHash},
                {"peer_id", peerId},
                {"port", std::to_string(port)},
                {"uploaded", std::to_string(0)},
                {"downloaded", std::to_string(0)},
                {"left", std::to_string(tf.length)},
                {"compact", std::to_string(1)}
        },
        cpr::Timeout{ 50000 }
    );
    std::string buffer = res.text;
    size_t i = 0;
    i++;
    while (i < buffer.size()) {
        if (buffer[i] == 'e') {
            i++;
        }
        else {
            std::string key = bencode::get_string(buffer, i);
            if (key == "peers") {
                
                std::string p = bencode::get_string(buffer, i);
                for (size_t j = 0; j < p.size(); j += 6) {
                    Peer peer;
                    peer.ip = std::to_string(static_cast<uint8_t>(p[j])) + "."
                        + std::to_string(static_cast<uint8_t>(p[j + 1])) + "."
                        + std::to_string(static_cast<uint8_t>(p[j + 2])) + "."
                        + std::to_string(static_cast<uint8_t>(p[j + 3]));
                    peer.port = (static_cast<uint16_t>(static_cast<uint8_t>(p[j + 4])) << 8) |
                        static_cast<uint16_t>(static_cast<uint8_t>(p[j + 5]));
                    peers_.push_back(peer);
                }
            }
            else {
                std::cerr << key << std::endl;
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
    if (peers_.empty()) {
        for (auto u : tf.announceList) {
            cpr::Response res = cpr::Get(
                cpr::Url{ u },
                cpr::Parameters{
                        {"info_hash", tf.infoHash},
                        {"peer_id", peerId},
                        {"port", std::to_string(port)},
                        {"uploaded", std::to_string(0)},
                        {"downloaded", std::to_string(0)},
                        {"left", std::to_string(tf.length)},
                        {"compact", std::to_string(1)}
                },
                cpr::Timeout{ 50000 }
            );
            std::string buffer = res.text;
            size_t i = 0;
            i++;
            while (i < buffer.size()) {
                if (buffer[i] == 'e') {
                    i++;
                }
                else {
                    std::string key = bencode::get_string(buffer, i);
                    if (key == "peers") {
                        
                        std::string p = bencode::get_string(buffer, i);
                        for (size_t j = 0; j < p.size(); j += 6) {
                            Peer peer;
                            peer.ip = std::to_string(static_cast<uint8_t>(p[j])) + "."
                                + std::to_string(static_cast<uint8_t>(p[j + 1])) + "."
                                + std::to_string(static_cast<uint8_t>(p[j + 2])) + "."
                                + std::to_string(static_cast<uint8_t>(p[j + 3]));
                            peer.port = (static_cast<uint16_t>(static_cast<uint8_t>(p[j + 4])) << 8) |
                                static_cast<uint16_t>(static_cast<uint8_t>(p[j + 5]));
                            peers_.push_back(peer);
                        }
                    }
                    else {
                        std::cerr << key << std::endl;
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
            if (!peers_.empty()) {
                break;
            }
        }
    }
}

const std::vector<Peer> &TorrentTracker::GetPeers() const {
    if (peers_.empty()) {
        throw std::invalid_argument("empty");
    }
    return peers_;
}
