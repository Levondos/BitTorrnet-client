#pragma once

#include "byte_tools.h"
#include <string>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdexcept>
#include <netinet/in.h>
#include <arpa/inet.h>



/*
 * Обертка над низкоуровневой структурой сокета.
 */
class TcpConnect {
public:
    TcpConnect(std::string ip, int16_t port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout);

    ~TcpConnect();

    /*
     * Установить tcp соединение.
     * Если соединение занимает более `connectTimeout` времени, то прервать подключение и выбросить исключение.
     */
    void EstablishConnection();

    /*
     * Послать данные в сокет
     */
    void SendData(const std::string& data) const;

    /*
     * Прочитать данные из сокета.
     * Если передан `bufferSize`, то прочитать `bufferSize` байт.
     * Если параметр `bufferSize` не передан, то сначала прочитать 4 байта, а затем прочитать количество байт, равное
     * прочитанному значению.
     * Если чтение занимает более readTimeout времени, то выкидывается исключение
     * Первые 4 байта (в которых хранится длина сообщения) интерпретируются как целое число в формате big endian,
     */
    std::string ReceiveData(size_t bufferSize = 0) const;

    /*
     * Закрыть сокет
     */
    void CloseConnection();

    const std::string& GetIp() const;

    int GetPort() const;
private:
    const std::string ip_;
    const int16_t port_;
    std::chrono::milliseconds connectTimeout_, readTimeout_;
    int sock_;
    int32_t ipv4_;
};
