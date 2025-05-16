#include "torrent_file.h"
#include <fstream>
#include <sstream>


TorrentFileCompiler::TorrentFileCompiler(const std::string& Str) : BencodeParser{Str} {
    fill_tf();
    //здесь должна быть проверка на укомплектованность TorrentFile
}

TorrentFile& TorrentFileCompiler::GetTorrentFile() {
    return tf_;
}

void TorrentFileCompiler::distrib_info(const std::string& key, const Bencode::any_type& value) {
    if (key == "name") {
        tf_.name = std::get<std::string>(value);
    }
    if (key == "piece length") {
        tf_.pieceLength = std::get<long long>(value);
    }
    if (key == "length") {
        tf_.length = std::get<long long>(value);
    }
    if (key == "pieces") {
        std::string pieces = std::get<std::string>(value);
        if (pieces.size() % 20 != 0) {
            throw std::runtime_error("Invalid pieces length");
        }
        tf_.pieceHashes.reserve(pieces.size() / 20);
        for (size_t i = 0; i < pieces.size(); i += 20) {
            tf_.pieceHashes.push_back(pieces.substr(i, 20));
        }
    }
}

void TorrentFileCompiler::distrib_main_fields(const std::string& key, Bencode::any_type& value) {
    try {
        if (key == "announce") {
            tf_.announce = std::get<std::string>(value);
        }
        if (key == "comment") {
            tf_.comment = std::get<std::string>(value);
        }
        if (key == "info") {
            auto& infoMap = std::get<Bencode::map_with_any_el>(value);
            for (auto it = infoMap.begin(); it != infoMap.end(); ++it) {
                distrib_info(it->first, it->second.value);
            }
        }
    } catch (std::bad_variant_access& e) {
        throw std::runtime_error("Invalid torrent file");
    }
}

void TorrentFileCompiler::fill_tf() {
    str_iter it = buffer_.begin();
    end_check(it);
    if (*it != 'd') {
        throw std::invalid_argument("Invalid torrent file format");
    }
    ++it;
    std::string infoStr;
    while (it != buffer_.end() && *it != 'e') {
        std::string key = read_str(it);
        str_iter begVal = it;
        Bencode::any_type value = read_any(it);
        str_iter endgVal = it;
        if (key == "info") {
            infoStr.assign(begVal, endgVal);
        }
        distrib_main_fields(key, value);
    }
    end_check(it);
    if (infoStr.empty()) {
        throw std::invalid_argument("Invalid torrent file. Excpected field 'info'");
    }
    tf_.infoHash = CalculateSHA1(infoStr);
}

std::string ReadTorrentFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file");
    }
    std::ostringstream oss;
    oss << file.rdbuf();
    if (file.fail()) {
        throw std::runtime_error("Failed to read file");
    }
    return oss.str();
}

TorrentFile LoadTorrentFile(const std::string& filename) {
    std::string bencode_str = ReadTorrentFile(filename);
    TorrentFileCompiler tfc(bencode_str);
    return tfc.GetTorrentFile();
}