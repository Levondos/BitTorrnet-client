#pragma once

#include <string>
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <map>
#include <sstream>
#include <exception>

#include "bencode.h"
#include "byte_tools.h"


/*структура для хранения данных из файла .torrent*/
struct TorrentFile {
    std::string announce;
    std::string comment;
    std::vector<std::string> pieceHashes;
    size_t pieceLength;
    size_t length;
    std::string name;
    std::string infoHash;
};


/*Одноразовый парсер файлов .torrent и компановщик структуры TorrentFile*/
class TorrentFileCompiler: private Bencode::BencodeParser{
public:
    TorrentFileCompiler() = delete;
    TorrentFileCompiler(const std::string& Str);
    
    /*Функция для получения TorrentFile*/
    TorrentFile& GetTorrentFile();

private:
    TorrentFile tf_;

    /*Функция для распределения ключ-значений по полю info*/
    void distrib_info(const std::string& key, const Bencode::any_type& value);
    
    /*Функция для распределения ключ-значений по основным полям структуры*/
    void distrib_main_fields(const std::string& key, Bencode::any_type& value);
    
    //Функция для заполнения tf_ для заданного buffer_
    void fill_tf();
};
    

/*  
Предполагается, что торрент файлы достаточно малы,
поэтому их полностью можно считать в локальную память
Здесь используется запись в поток и после конвертирование в std::string,
в дальнейшем можно доработать
*/
std::string ReadTorrentFile(const std::string& filename);

/*Функция парсит .torrent файл и загружает информацию из него в структуру `TorrentFile`*/
TorrentFile LoadTorrentFile(const std::string& filename);