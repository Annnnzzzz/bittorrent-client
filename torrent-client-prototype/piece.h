#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <iostream>
#include "byte_tools.h"

/*
 * Части файла скачиваются не за одно сообщение, а блоками размером 2^14 байт или меньше (последний блок обычно меньше)
 */
struct Block {

    enum Status {
        Missing = 0,
        Pending,
        Retrieved,
    };

    uint32_t piece;  // id части файла, к которой относится данный блок
    uint32_t offset;  // смещение начала блока относительно начала части файла в байтах
    uint32_t length;  // длина блока в байтах
    Status status;  // статус загрузки данного блока
    std::string data;  // бинарные данные
};

/*
 * Часть скачиваемого файла
 */
class Piece {
public:
    /*
     * index -- номер части файла, нумерация начинается с 0
     * length -- длина части файла. Все части, кроме последней, имеют длину, равную `torrentFile.pieceLength`
     * hash -- хеш-сумма части файла, взятая из `torrentFile.pieceHashes`
     */
    Piece(size_t index, size_t length, std::string hash) :
        index_(index), length_(length), hash_(hash) {
        int i = 0;
        while (i < (length_ / (1 << 14))) {
            Block b;
            b.piece = index_;
            b.offset = i * (1 << 14);
            b.length = (1 << 14);
            b.status = Block::Status::Missing;
            blocks_.push_back(b);
            i++;
        }
        if ((length_ % (1 << 14)) != 0) {
            Block b;
            b.piece = index_;
            b.offset = i * (1 << 14);
            b.length = (length_ % (1 << 14));
            b.status = Block::Status::Missing;
            blocks_.push_back(b);
        }
    }
    ~Piece() {};
    /*
     * Совпадает ли хеш скачанных данных с ожидаемым
     */
    bool HashMatches() const {
        return (GetDataHash() == CalculateSHA1(GetData()));
    }
    /*
     * Дать указатель на отсутствующий (еще не скачанный и не запрошенный) блок
     */
    Block* FirstMissingBlock() {
        for (auto& b : blocks_) {
            if (b.status == Block::Status::Missing) {
                return &b;
            }
        }
        return nullptr;
    }

    /*
     * Получить порядковый номер части файла
     */
    size_t GetIndex() const {
        return index_;
    }

    size_t GetLength() const {
        return length_;
    }

    /*
     * Сохранить скачанные данные для какого-то блока
     */
    void SaveBlock(size_t blockOffset, std::string data) {
        for (auto& b : blocks_) {
            if (b.offset == blockOffset) {
                b.data = data;
                b.status = Block::Status::Retrieved;
                return;
            }
        }
    }

    /*
     * Скачали ли уже все блоки
     */
    bool AllBlocksRetrieved() const {
        for (const auto& b : blocks_) {
            if (b.status != Block::Status::Retrieved) {
                return false;
            }
        }
        return true;
    }

    /*
     * Получить скачанные данные для части файла
     */
    std::string GetData() const {
        std::string h;
        for (const auto& b : blocks_) {
            h += b.data;
        }
        return h;
    }

    /*
     * Посчитать хеш по скачанным данным
     */
    std::string GetDataHash() const {
        std::string h = GetData();
        return CalculateSHA1(h);
    }

    /*
     * Получить хеш для части из .torrent файла
     */
    const std::string& GetHash() const {
        return hash_;
    }

    /*
     * Удалить все скачанные данные и отметить все блоки как Missing
     */
    void Reset() {
        for (auto& b : blocks_) {
            b.data = "";
            b.status = Block::Status::Missing;
        }
    }

private:
    const size_t index_, length_;
    const std::string hash_;
    std::vector<Block> blocks_;
};

using PiecePtr = std::shared_ptr<Piece>;
