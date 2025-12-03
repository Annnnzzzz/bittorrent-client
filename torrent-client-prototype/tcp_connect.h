#pragma once

#include "byte_tools.h"
#include <string>
#include <chrono>
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>   
#include <unistd.h>      
#include <fcntl.h>  

/*
 * Обертка над низкоуровневой структурой сокета.
 */
class TcpConnect {
public:
    TcpConnect(std::string ip, int port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout):
        ip_(ip), port_(port), connectTimeout_(connectTimeout), readTimeout_(readTimeout) {
        sock_ = socket(AF_INET, SOCK_STREAM, 0);
        struct timeval tv;
        tv.tv_sec = readTimeout_.count() / 1000;
        tv.tv_usec = (readTimeout_.count() % 1000) * 1000;
        setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }
    ~TcpConnect() {
        if (sock_ >= 0) {
            close(sock_);
            sock_ = -1;
        }
    }

    /*
     * Установить tcp соединение.
     * Если соединение занимает более `connectTimeout` времени, то прервать подключение и выбросить исключение.
     * Полезная информация:
     * - https://man7.org/linux/man-pages/man7/socket.7.html
     * - https://man7.org/linux/man-pages/man2/connect.2.html
     * - https://man7.org/linux/man-pages/man2/fcntl.2.html (чтобы включить неблокирующий режим работы операций)
     * - https://man7.org/linux/man-pages/man2/select.2.html
     * - https://man7.org/linux/man-pages/man2/setsockopt.2.html
     * - https://man7.org/linux/man-pages/man2/close.2.html
     * - https://man7.org/linux/man-pages/man3/errno.3.html
     * - https://man7.org/linux/man-pages/man3/strerror.3.html
     */
    void EstablishConnection() {
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(ip_.c_str());
        address.sin_port = htons(port_);

        int flags = fcntl(sock_, F_GETFL, 0);
        fcntl(sock_, F_SETFL, flags | O_NONBLOCK);

        int res = connect(sock_, (struct sockaddr*)&address, sizeof(address));
        if (res == 0) {
            fcntl(sock_, F_SETFL, flags);
            return;
        }

        fd_set wfds;

        FD_ZERO(&wfds);
        FD_SET(sock_, &wfds);

        struct timeval tv;
        tv.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(connectTimeout_).count();
        tv.tv_usec = (connectTimeout_.count() % 1000) * 1000;

        int retval = select(sock_ + 1, NULL, &wfds, NULL, &tv);
        if (retval <= 0) {
            close(sock_);
            sock_ = -1;
            throw std::runtime_error("Connection timed out");
        }

        fcntl(sock_, F_SETFL, flags);
    }

    /*
     * Послать данные в сокет
     * Полезная информация:
     * - https://man7.org/linux/man-pages/man2/send.2.html
     */
    void SendData(const std::string& data) const {
        if (sock_ == -1) {
            throw std::runtime_error("Invalid socket descriptor");
        }
        if (send(sock_, data.c_str(), data.size(), 0) <= 0) {
            throw std::runtime_error("Failed to send request");
        }
    }

    /*
     * Прочитать данные из сокета.
     * Если передан `bufferSize`, то прочитать `bufferSize` байт.
     * Если параметр `bufferSize` не передан, то сначала прочитать 4 байта, а затем прочитать количество байт, равное
     * прочитанному значению.
     * Первые 4 байта (в которых хранится длина сообщения) интерпретируются как целое число в формате big endian,
     * см https://wiki.theory.org/BitTorrentSpecification#Data_Types
     * Полезная информация:
     * - https://man7.org/linux/man-pages/man2/poll.2.html
     * - https://man7.org/linux/man-pages/man2/recv.2.html
     */
    std::string ReceiveData(size_t bufferSize = 0) const {
        std::string data, ans;
        if (bufferSize == 0) {
            char lengthBuf[4];
            size_t received = 0;
            while (received < 4) {
                int res = recv(sock_, lengthBuf + received, 4 - received, 0);
                if (res <= 0) throw std::runtime_error("Read error");
                received += res;
            }
            bufferSize = BytesToInt(std::string_view(lengthBuf, 4));
            ans.append(lengthBuf, 4);
        }
        data.resize(bufferSize);
        size_t received = 0;
        while (received < bufferSize) {
            int res = recv(sock_, data.data() + received, bufferSize - received, 0);
            if (res <= 0) throw std::runtime_error("Read error");
            received += res;
        }
        //recv(sock_, data.data(), bufferSize, 0);
        ans.append(data);
        return ans;
    }

    /*
     * Закрыть сокет
     */
    void CloseConnection() {
        if (sock_ > 0) {
            close(sock_);
            sock_ = -1;
        }
    }

    const std::string& GetIp() const;
    int GetPort() const;
private:
    const std::string ip_;
    const int port_;
    std::chrono::milliseconds connectTimeout_, readTimeout_;
    int sock_=-1;
};
