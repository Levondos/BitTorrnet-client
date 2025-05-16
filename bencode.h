#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>

namespace Bencode {

/*
 * В это пространство имен рекомендуется вынести функции для работы с данными в формате bencode.
 * Этот формат используется в .torrent файлах и в протоколе общения с трекером
 */


    /*  Тип данных, хранящийся в bencode
    это могут быть: строки, числа, словари от тех же примитивов и других словарей и списков, списки из словарей, примитивов и других списков.
        Так как типы могут рекурсивно вкладываться друг в друга, то воспользуемся небольшим трюком -
    создадим обертку на тип и сделаем variant от std::vector<any_struct> и std::map<std::string, any_struct>,
    т.к. списки и словари не могут бесконечно рекурсивно вкладываться (то есть мы когда-нибудь дойдем до момента,
    когда списки и словари состоят из примитивов), то мы обещаем компилятору, что он сможет подставить типы,
    а пока он просто верит, что такое сделать можно.
    */
    struct any_struct;
    using any_type = std::variant<
        std::string,
        long long,
        std::vector<any_struct>,
        std::map<std::string, any_struct>
    >;
    struct any_struct {
        any_type value;

        any_struct(): value{0LL}{} // нулевое значение(небольшой костыль)
        any_struct(any_type Value): value{Value}{}
    };


    using map_with_any_el = std::map<std::string, any_struct>;
    using vector_with_any_el = std::vector<any_struct>;

    /*Одноразовый базовый парсер Bencode файлов.
    Реализация допускает наследование от него для специфичных парсеров
    Кроме методов для парсинга содержит словарь, который формируется лениво*/
    class BencodeParser{
    public:
        BencodeParser() = delete;
        explicit BencodeParser(const std::string& Str);

        map_with_any_el& GetTorrentDict();
    protected:
        const std::string buffer_;          //data
        
        using str_iter = std::string::const_iterator;

        /*Функция для проверки итератора на конец строки*/
        void end_check(const str_iter& it);

        /*Функция для чтения длин строк*/
        std::size_t read_len(str_iter& it);
        
        /*Функция для чтения строк, причем длину строку так же считает сама*/
        std::string read_str(str_iter& it);

        /*Функция для чтения целых чисел*/
        long long read_num(str_iter& it);

        /*Функция для чтения словарей*/
        map_with_any_el read_dict(str_iter& it);

        /*Функция для чтения списков*/
        vector_with_any_el read_list(str_iter& it);

        /*Функция для чтения любого типа в bencode*/
        any_type read_any(str_iter& it);
    private:
        bool dict_ready_;
        map_with_any_el torrent_dict_;
        
        void fill_torrent_dict();
    };

}
