#include "piece_storage.h"
#include <iostream>

PiecePtr PieceStorage::GetNextPieceToDownload() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (remainPieces_.empty()) {
        return nullptr;
    }
    auto p = remainPieces_.front();
    remainPieces_.pop_front();
    PiecesInProgress++;
    return p;
}

void PieceStorage::PieceProcessed(const PiecePtr& piece) {
    //std::lock_guard<std::mutex> lock(mutex_);
    if (piece->AllBlocksRetrieved()) {
        std::lock_guard<std::mutex> lock(mutex_);
        SavePieceToDisk(piece);
    }
}

bool PieceStorage::QueueIsEmpty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return (remainPieces_.empty());
}

size_t PieceStorage::TotalPiecesCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    /*if (true) {
        throw std::runtime_error("TotalPiecesCount");
    }*/
    return TotalPieces;
}

size_t PieceStorage::PiecesSavedToDiscCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return PiecesSavedToDisc;
}

void PieceStorage::CloseOutputFile() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (outputFile_.is_open()) {
        outputFile_.close();
    }
}
void PieceStorage::AddPieceInQueue(PiecePtr piece) {
    std::lock_guard<std::mutex> lock(mutex_);
    remainPieces_.push_front(piece);
    return;
}
const std::vector<size_t>& PieceStorage::GetPiecesSavedToDiscIndices() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return PiecesSavedToDiscIndices;
}

size_t PieceStorage::PiecesInProgressCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return PiecesInProgress;
}

void PieceStorage::SavePieceToDisk(const PiecePtr& piece) {
    for (auto i : PiecesSavedToDiscIndices) {
        if (piece->GetIndex() == i) {
            return;
        }
    }
    std::cout << "Downloaded piece " << piece->GetIndex() << std::endl;

    //outputFile_.seekp(piece->GetIndex() * piece->GetLength());
    const auto pos = static_cast<uint64_t>(piece->GetIndex()) * piece->GetLength();
    if (pos > std::numeric_limits<std::streamoff>::max()) {
        throw std::runtime_error("File position overflow");
    }
    outputFile_.seekp(static_cast<std::streamoff>(pos));
   
    outputFile_.write(reinterpret_cast<const char*>(piece->GetData().data()),piece->GetData().size());
    
    outputFile_.flush();
    
    PiecesSavedToDisc++;
    PiecesInProgress--;
    PiecesSavedToDiscIndices.push_back(piece->GetIndex());
}
