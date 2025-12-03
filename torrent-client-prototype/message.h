#pragma once

#include <cstdint>
#include <string>

#include "byte_tools.h"


/*
 * Тип сообщения в протоколе торрента.
 * https://wiki.theory.org/BitTorrentSpecification#Messages
 */
enum class MessageId : uint8_t {
    Choke = 0,
    Unchoke,
    Interested,
    NotInterested,
    Have,
    BitField,
    Request,
    Piece,
    Cancel,
    Port,
    KeepAlive,
};

struct Message {
    MessageId id;
    size_t messageLength;
    std::string payload;

    /*
     * Выделяем тип сообщения и длину и создаем объект типа Message.
     * Подразумевается, что здесь в качестве `messageString` будет приниматься строка, прочитанная из TCP-сокета
     */
    static Message Parse(const std::string& messageString) {
        Message m;
        m.messageLength = BytesToInt(messageString.substr(0, 4));
        if (m.messageLength == 0) {
            m.id = MessageId::KeepAlive;
            return m;
        }
        uint8_t id_ = static_cast<uint8_t>(messageString[4]);
        m.id = static_cast<MessageId>(id_);

        if (m.messageLength > 1) {
            m.payload = messageString.substr(5, m.messageLength - 1);
        }
        if (m.id == MessageId::Piece) {
            int id = BytesToInt(messageString.substr(5, 4));
            int begin = BytesToInt(messageString.substr(9, 4));

        }
        return m;
    }

    /*
     * Создаем сообщение с заданным типом и содержимым. Длина вычисляется автоматически
     */
    static Message Init(MessageId id, const std::string& payload) {
        Message m;
        m.id = id;
        if (id == MessageId::KeepAlive) {
            m.messageLength = 0;
            return m;
        }
        m.payload = payload;
        m.messageLength = payload.size() + 1;
        return m;
    }

    /*
     * Формируем строку с сообщением, которую можно будет послать пиру в соответствии с протоколом.
     * Получается строка вида "<1 + payload length><message id><payload>"
     * Секция с длиной сообщения занимает 4 байта и представляет собой целое число в формате big-endian
     * id сообщения занимает 1 байт и может принимать значения от 0 до 9 включительно
     */
    std::string ToString() const {
        uint32_t length = static_cast<uint32_t>(messageLength);
        std::string s = IntToBytes(length);
        /*
        s += static_cast<char>((length >> 24) & 0xFF);
        s += static_cast<char>((length >> 16) & 0xFF);
        s += static_cast<char>((length >> 8) & 0xFF);
        s += static_cast<char>(length & 0xFF);*/
        if (messageLength == 0) {
            return s;
        }
        s += static_cast<char>(((static_cast<uint8_t>(id))) & 0xFF);
        s += payload;
        return s;
    }
};
