#include "torrent_tracker.h"
#include <cpr/cpr.h>
#include <sstream>

TorrentTracker::TorrentTracker(const std::string& url) : url_{url} {}

void TorrentTracker::UpdatePeers(const TorrentFile& tf, std::string peerId, int port) {
    if (peerId.size() != 20) {
        throw std::invalid_argument("'peer_id' must be exactly 20 bytes");
    }

    cpr::Response res = cpr::Get(
        cpr::Url{tf.announce},
        cpr::Parameters {
            {"info_hash", tf.infoHash},
            {"peer_id", peerId},
            {"port", std::to_string(port)},
            {"uploaded", std::to_string(0)},
            {"downloaded", std::to_string(0)},
            {"left", std::to_string(tf.length)},
            {"compact", std::to_string(1)}
        },
        cpr::Timeout{20000}
    );

    if (res.status_code == 200) {
        if (res.text.find("failure reason") == std::string::npos) {
            peers_.clear();
            SearchPeersInBencode(res.text);
            return;
        }
    }

    std::ostringstream oss;
    oss << "Server responded '" << res.text << "'" << '\n';
    oss << "Possibly you have parsed a .torrent file wrong hence the request for peers is wrong too" << '\n';
    oss << "Tried GET " << res.url << '\n';
    oss << "Error: " << (int)res.error.code << " - " << res.error.message << '\n';
    if (res.error) {
        oss << "Tracker response: " << res.status_code << " " << res.status_line << ": " << res.text << '\n';
    }
    oss << "Info hash: ";
    for (char c : tf.infoHash) {
        oss << (unsigned int)(unsigned char)c << " ";
    }

    throw std::invalid_argument(oss.str());
}

const std::vector<Peer>& TorrentTracker::GetPeers() const {
    return peers_;
}

std::string TorrentTracker::convert_bytes_to_str_uint(char c) {
    return std::to_string(static_cast<uint8_t>(c));
}

std::string TorrentTracker::string_to_ip(const std::string& Str) {
    if (Str.size() != 4) {
        throw std::invalid_argument("IP must consist of 4 bytes");
    }

    std::string IP = convert_bytes_to_str_uint(Str[0]) + "." +
                     convert_bytes_to_str_uint(Str[1]) + "." +
                     convert_bytes_to_str_uint(Str[2]) + "." +
                     convert_bytes_to_str_uint(Str[3]);

    return IP;
}

uint16_t TorrentTracker::string_to_port(const std::string& Str) {
    if (Str.size() != 2) {
        throw std::invalid_argument("Port must consist of 2 bytes");
    }

    uint16_t port = (static_cast<uint8_t>(Str[0]) << 8) | static_cast<uint8_t>(Str[1]);

    return port;
}

void TorrentTracker::SearchPeersInBencode(const std::string& bencode) {
    Bencode::BencodeParser dc(bencode);
    auto dict = dc.GetTorrentDict();

    if (dict.find("peers") == dict.end()) {
        throw std::invalid_argument("Peers not in response");
    }
    auto any_value = dict["peers"].value;
    if (!std::holds_alternative<std::string>(any_value)) {
        throw std::invalid_argument("Expected std::string by key 'peers'");
    }

    PeerStringCompactParse(std::get<std::string>(any_value));
}

void TorrentTracker::PeerStringCompactParse(const std::string& peer_str) {
    if (peer_str.size() % 6 != 0) {
        throw std::invalid_argument("Compact peer list length is not a multiple of 6");
    }

    for (std::size_t i = 0; i < peer_str.size(); i += 6) {
        std::string IP = string_to_ip(peer_str.substr(i, 4));
        int port = static_cast<int>(string_to_port(peer_str.substr(i + 4, 2)));
        peers_.emplace_back(IP, port);
    }
}