#include "bencode.h"

namespace Bencode {
/*BencodeParser---------------------------------------------------------------------------*/
//public:    
    BencodeParser::BencodeParser(const std::string& Str) : 
        buffer_{Str},
        dict_ready_{false}
        {}

    map_with_any_el& BencodeParser::GetTorrentDict(){
        if (!dict_ready_){
            dict_ready_ = true;
            fill_torrent_dict();
        }
        return torrent_dict_;
    }

//protected:
    void BencodeParser::end_check(const str_iter& it) {
        if (it == buffer_.end()) {
            throw std::invalid_argument("Invalid torrent file");
        }
    }

    std::size_t BencodeParser::read_len(str_iter& it) {
        str_iter begin_len = it;
        while (it != buffer_.end() && isdigit(*it)) {
            ++it;
        }
        end_check(it);
        if (*it != ':') {
            throw std::invalid_argument("Invalid torrent file");
        }
        std::string len(begin_len, it);
        ++it;
        return std::stoull(len);
    }

    std::string BencodeParser::read_str(str_iter& it) {
        std::size_t sizeStr = read_len(it);
        str_iter beg_str = it;
        for (int i = 0; i < sizeStr; ++i) {
            end_check(it);
            ++it;
        }
        std::string str(beg_str, it);
        return str;
    }

    long long BencodeParser::read_num(str_iter& it) {
        str_iter beg_num = it;
        while (it != buffer_.end() && *it != 'e') {
            ++it;
        }
        end_check(it);
        std::string num(beg_num, it);
        ++it;
        return std::stoll(num);
    }

    map_with_any_el BencodeParser::read_dict(str_iter& it) {
        map_with_any_el Dict;
        while (it != buffer_.end() && *it != 'e') {
            std::string key = read_str(it);
            any_type value = read_any(it);
            Dict.emplace(key, value);
        }
        end_check(it);
        ++it;
        return Dict;
    }

    vector_with_any_el BencodeParser::read_list(str_iter& it) {
        vector_with_any_el Vec;
        while (it != buffer_.end() && *it != 'e') {
            any_type el = read_any(it);
            Vec.push_back(el);
        }
        end_check(it);
        ++it;
        return Vec;
    }

    any_type BencodeParser::read_any(str_iter& it) {
        end_check(it);
        char id = *it;
        switch (id) {
            case 'i':
                return read_num(++it);
            case 'l':
                return read_list(++it);
            case 'd':
                return read_dict(++it);
            default:
                if (isdigit(*it)) {
                    return read_str(it);
                }
                throw std::invalid_argument("Invalid torrent file");
        }
    }

//private:
    void BencodeParser::fill_torrent_dict() {
        str_iter it = buffer_.begin();
        end_check(it);
        if (*it != 'd') {
            throw std::runtime_error("Invalid torrent file");
        }
        ++it;
        while (it != buffer_.end() && *it != 'e') {
            std::string key = read_str(it);
            any_type value = read_any(it);
            torrent_dict_[std::move(key)] = std::move(value);
        }
        end_check(it);
    }
/*----------------------------------------------------------------------------------------*/

}
