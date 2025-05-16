#pragma once

#include "torrent_file.h"
#include "peer.h"
#include <cpr/cpr.h>


class TorrentTracker {
public:
    /*
     * url - адрес трекера, берется из поля announce в .torrent-файле
     */
    TorrentTracker(const std::string& url);

    /*
     * Получить список пиров у трекера и сохранить его для дальнейшей работы.
     * Запрос пиров происходит посредством HTTP GET запроса, данные передаются в формате bencode.
     * Такой же формат использовался в .torrent файле.
     */
    void UpdatePeers(const TorrentFile& tf, std::string peerId, int port);

    /*
     * Отдает полученный ранее список пиров
     */
    const std::vector<Peer>& GetPeers() const;

private:
    std::string url_;
    std::vector<Peer> peers_;

    std::string convert_bytes_to_str_uint(char c);

    std::string string_to_ip(const std::string& Str);

    uint16_t string_to_port(const std::string& Str);

    void SearchPeersInBencode(const std::string& bencode);

    void PeerStringCompactParse(const std::string& peer_str);
};

