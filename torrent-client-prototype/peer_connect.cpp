#include "byte_tools.h"
#include "peer_connect.h"
#include "message.h"
#include "torrent_file.h"
#include <sstream>
#include <utility>
#include <cassert>
#include <chrono>
#include <exception>

using namespace std::chrono_literals;

//std::mutex globalMutex;

PeerConnect::PeerConnect(const Peer& peer, const TorrentFile& tf, std::string selfPeerId, PieceStorage& pieceStorage) :
    tf_(tf),
    socket_(peer.ip, peer.port, std::chrono::milliseconds(3000), std::chrono::milliseconds(3000)),
    selfPeerId_(selfPeerId),
    peerId_("0"),
    terminated_(false),
    choked_(true),
    piecesAvailability_(PeerPiecesAvailability()),
    pieceStorage_(pieceStorage),
    pieceInProgress_(pieceStorage.GetNextPieceToDownload()),
    pendingBlock_(false) {
}

void PeerConnect::Run() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        failed_ = false;
        terminated_ = false;
    }
    /*if (pieceStorage_.PiecesSavedToDiscCount() == 20) {
        return;
    }*/
    std::unique_lock<std::mutex> lock(mutex_);
    while (!terminated_) {
        
        if (EstablishConnection()) {
            lock.unlock();
            MainLoop();
            lock.lock();
            terminated_ = true;
            socket_.CloseConnection();
        }
        else {
            socket_.CloseConnection();
            terminated_ = true;
            failed_ = true;
        }
    }
}

void PeerConnect::PerformHandshake() {
    try {
        socket_.EstablishConnection();
    }
    catch (...) {
        
    }
    
    std::string handshake;
    handshake += static_cast<char>((19) & 0xFF);
    handshake += "BitTorrent protocol";
    for (int i = 0; i < 8; ++i) {
        handshake += static_cast<char>((0) & 0xFF);
    }
    handshake += tf_.infoHash;
    handshake += selfPeerId_;
    
    try {
        socket_.SendData(handshake);
    }
    catch (...) {
        throw std::runtime_error("Invalid socket descriptor");
    }
    std::string ans = socket_.ReceiveData(68);
    if (ans.size() != 68 || ans.substr(1, 19) != "BitTorrent protocol" ||
        ans.substr(28, 20) != tf_.infoHash) {
        throw std::runtime_error("Invalid handshake");
    }
    peerId_ = ans.substr(48, 20);
    
}

bool PeerConnect::EstablishConnection() {
    try {
        PerformHandshake();
        ReceiveBitfield();
        SendInterested();
        return true;
    }
    catch (const std::exception& e) {
        
        return false;
    }
}

void PeerConnect::ReceiveBitfield() {
   
    std::string ans = socket_.ReceiveData();
    Message m = Message::Parse(ans);
    if (m.id == MessageId::BitField) {
        PeerPiecesAvailability p(m.payload);
        piecesAvailability_ = p;
    }
    else if (m.id == MessageId::Unchoke) {
        choked_ = false;
    }
    int g = 0;
    for (size_t h = 0; h < piecesAvailability_.Size(); h++) {
        if (piecesAvailability_.IsPieceAvailable(h)) {
            pieceInProgress_ = std::make_shared<Piece>(h, tf_.pieceLength, tf_.pieceHashes[h]);
            g++;
            break;
        }
    }
    if (g == 0) {
        Terminate();
    }
}

void PeerConnect::SendInterested() {
    
    Message m = Message::Init(MessageId::Interested, "");
    std::string send = m.ToString();
    socket_.SendData(send);
}

void PeerConnect::RequestPiece() {
    
    PiecePtr piece = pieceInProgress_;
    if (!piece) {
        Terminate();
        return;
    }

    Block* block = piece->FirstMissingBlock();
    if (!block) {
        pendingBlock_ = false;
        
        return;
    }
    std::string s;
    s = IntToBytes(static_cast<uint32_t>(block->piece)) +
        IntToBytes(static_cast<uint32_t>(block->offset)) +
        IntToBytes(static_cast<uint32_t>(block->length));
    Message m = Message::Init(MessageId::Request, s);
    std::string send = m.ToString();
    
    socket_.SendData(send);

    block->status = Block::Pending;
    pendingBlock_ = true;
}


bool PeerConnect::Failed()  {
    std::lock_guard<std::mutex> lock(mutex_);
    return this->failed_;
}

void PeerConnect::Terminate() {
    std::lock_guard<std::mutex> lock(mutex_);
    this->terminated_ = true;
    this->failed_ = true;
}

void PeerConnect::MainLoop() {
    using namespace std::chrono;
    auto lastActivityTime = steady_clock::now();
    std::unique_lock<std::mutex> lock(mutex_);
    while (!terminated_) {
        lock.unlock();
        auto now = steady_clock::now();
        if (duration_cast<milliseconds>(now - lastActivityTime).count() >= 120000) {
            Terminate(); 
            break;
        }
        std::string ans = socket_.ReceiveData();
        Message m = Message::Parse(ans);
        
        if (m.id == MessageId::Have) {
            uint32_t pieceIndex = BytesToInt(m.payload);
            piecesAvailability_.SetPieceAvailability(pieceIndex);
            
            lastActivityTime = steady_clock::now();
        }
        else if (m.id == MessageId::Piece) {
            
            pendingBlock_ = false;
            uint32_t id = BytesToInt(m.payload.substr(0, 4));
            
            uint32_t begin = BytesToInt(m.payload.substr(4, 4));
            std::string blockData = m.payload.substr(8);
            pieceInProgress_->SaveBlock(begin, blockData);
            if (pieceInProgress_->AllBlocksRetrieved()) {
                pieceStorage_.PieceProcessed(pieceInProgress_);
                int g = 0;
                for (size_t h = pieceInProgress_->GetIndex()+1; h < piecesAvailability_.Size(); h++) {
                    if (piecesAvailability_.IsPieceAvailable(h)) {
                       
                        pieceInProgress_ = std::make_shared<Piece>(h, tf_.pieceLength, tf_.pieceHashes[h]);
                        g++; 
                        std::cout << "next "<< pieceInProgress_->GetIndex() << std::endl;
                        break;
                    }
                }
                if (g == 0) {
                    Terminate();
                }
                
            }
            lastActivityTime = steady_clock::now();
        }
        else if (m.id == MessageId::Unchoke) {
           
            choked_ = false;
            lastActivityTime = steady_clock::now();
        }
        else if (m.id == MessageId::Choke) {
            
            choked_ = true;
            lastActivityTime = steady_clock::now();
        }
        if (!choked_ && !pendingBlock_) {
            
            RequestPiece();
            lastActivityTime = steady_clock::now();
        }
        else if (choked_) {
            Terminate();
            lastActivityTime = steady_clock::now();
        }
        lock.lock();
    }
}


