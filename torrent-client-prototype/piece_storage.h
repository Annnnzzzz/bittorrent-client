#pragma once

#include "torrent_file.h"
#include "piece.h"
#include <queue>
#include <string>
#include <unordered_set>
#include <mutex>
#include <fstream>
#include <filesystem>
#include <deque>
/*
 * Хранилище информации о частях скачиваемого файла.
 * В этом классе отслеживается информация о том, какие части файла осталось скачать
 */
class PieceStorage {
public:
    PieceStorage(const TorrentFile& tf, const std::filesystem::path& outputDirectory){
        TotalPieces = tf.pieceHashes.size();
        size_t size = tf.pieceLength;
        for (size_t i = 0; i < TotalPieces - 1; ++i) {
            PiecePtr p = std::make_shared<Piece>(i, size, tf.pieceHashes[i]);
            remainPieces_.push_back(p);
        }
        size = tf.length - (TotalPieces - 1) * size;
        PiecePtr p = std::make_shared<Piece>(TotalPieces - 1, size, tf.pieceHashes[TotalPieces - 1]);
        remainPieces_.push_back(p);
        if (!std::filesystem::exists(outputDirectory)) {
            std::filesystem::create_directories(outputDirectory);
        }
        outputPath_ = outputDirectory / tf.name;
        outputFile_.open(outputPath_, std::ios::binary | std::ios::out);
        if (!outputFile_.is_open()) {
            throw std::runtime_error("Failed to open output file");
        }
        outputFile_.seekp(tf.length - 1);
        outputFile_.put('\0');

    }
    ~PieceStorage() {
        CloseOutputFile();
    }

    /*
     * Отдает указатель на следующую часть файла, которую надо скачать
     */
    PiecePtr GetNextPieceToDownload();

    /*
     * Эта функция вызывается из PeerConnect, когда скачивание одной части файла завершено.
     * В рамках данного задания требуется очистить очередь частей для скачивания как только хотя бы одна часть будет успешно скачана.
     */
    void PieceProcessed(const PiecePtr& piece);

    /*
     * Остались ли нескачанные части файла?
     */
    bool QueueIsEmpty() const;

    /*
     * Сколько частей файла было сохранено на диск
     */
    size_t PiecesSavedToDiscCount() const;

    /*
     * Сколько частей файла всего
     */
    size_t TotalPiecesCount() const;

    /*
     * Закрыть поток вывода в файл
     */
    void CloseOutputFile();

    void AddPieceInQueue(PiecePtr piece);

    /*
     * Отдает список номеров частей файла, которые были сохранены на диск
     */
    const std::vector<size_t>& GetPiecesSavedToDiscIndices() const;

    /*
     * Сколько частей файла в данный момент скачивается
     */
    size_t PiecesInProgressCount() const;

private:
    std::filesystem::path outputPath_;
    std::ofstream outputFile_;
    std::deque<PiecePtr> remainPieces_;
    size_t TotalPieces = 0;
    mutable std::mutex mutex_;
    size_t PiecesSavedToDisc = 0;
    size_t PiecesInProgress = 0;
    std::vector<size_t> PiecesSavedToDiscIndices;
    /*
     * Сохраняет данную скачанную часть файла на диск.
     * Сохранение всех частей происходит в один выходной файл. Позиция записываемых данных зависит от индекса части
     * и размера частей. Данные, содержащиеся в части файла, должны быть записаны сразу в правильную позицию.
     */
    void SavePieceToDisk(const PiecePtr& piece);
};
