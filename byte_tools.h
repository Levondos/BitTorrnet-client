#pragma once

#include <string>
#include <cstdint>
/*
 * Преобразовать 4 байта в формате big endian в int
 */
int32_t BytesToInt(std::string_view bytes);

/*
 * Расчет SHA1 хеш-суммы. Здесь в результате подразумевается не человеко-читаемая строка, а массив из 20 байтов
 * в том виде, в котором его генерирует библиотека OpenSSL
 */
std::string CalculateSHA1(const std::string& msg);

/*
 * Преобразовать 4-битное число в std::string в формате big endian
 */
std::string IntToBytes(uint32_t n);