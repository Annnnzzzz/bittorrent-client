#include "torrent_tracker.h"
#include "piece_storage.h"
#include "peer_connect.h"
#include "byte_tools.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <random>
#include <thread>
#include <algorithm>
#include <cstdlib>

namespace fs = std::filesystem;

std::mutex cerrMutex, coutMutex;

std::string RandomString(size_t length) {
    std::random_device random;
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result.push_back(random() % ('Z' - 'A') + 'A');
    }
    return result;
}

const std::string PeerId = "TESTAPPDONTWORRY" + RandomString(4);
constexpr size_t PiecesToDownload = 20;

void CheckDownloadedPiecesIntegrity(const std::filesystem::path& outputFilename, const TorrentFile& tf, PieceStorage& pieces) {
    //pieces.CloseOutputFile();



    std::vector<size_t> pieceIndices = pieces.GetPiecesSavedToDiscIndices();
    std::sort(pieceIndices.begin(), pieceIndices.end());

    std::ifstream file(outputFilename, std::ios_base::binary);
    for (size_t pieceIndex : pieceIndices) {
        if (pieceIndex > 35) {
            break;
        }
        const std::streamoff positionInFile = pieceIndex * tf.pieceLength;
        file.seekg(positionInFile);
        if (!file.good()) {
            throw std::runtime_error("Cannot read from file");
        }
        std::string pieceDataFromFile(tf.pieceLength, '\0');
        file.read(pieceDataFromFile.data(), tf.pieceLength);
        const size_t readBytesCount = file.gcount();
        pieceDataFromFile.resize(readBytesCount);
        const std::string realHash = CalculateSHA1(pieceDataFromFile);

        if (realHash != tf.pieceHashes[pieceIndex]) {
            std::cerr << "File piece with index " << pieceIndex << " started at position " << positionInFile <<
                " with length " << pieceDataFromFile.length() << " has wrong hash " << std::endl;
        }
    }
}

void DeleteDownloadedFile(const std::filesystem::path& outputFilename) {
    std::filesystem::remove(outputFilename);
}

std::filesystem::path PrepareDownloadDirectory(const std::string& randomString) {
    std::filesystem::path outputDirectory = "/tmp/downloads";
    outputDirectory /= randomString;
    std::filesystem::create_directories(outputDirectory);
    return outputDirectory;
}

bool RunDownloadMultithread(PieceStorage& pieces, const TorrentFile& torrentFile, const std::string& ourId, const TorrentTracker& tracker) {
    using namespace std::chrono_literals;

    std::vector<std::thread> peerThreads;
    std::vector<std::shared_ptr<PeerConnect>> peerConnections;

    for (const Peer& peer : tracker.GetPeers()) {
        peerConnections.emplace_back(std::make_shared<PeerConnect>(peer, torrentFile, ourId, pieces));
    }

    peerThreads.reserve(peerConnections.size());
    for (auto& peerConnectPtr : peerConnections) {
        peerThreads.emplace_back(
            [peerConnectPtr]() {
                bool tryAgain = true;
                int attempts = 0;
                do {
                    try {
                        ++attempts;
                        peerConnectPtr->Run();
                    }
                    catch (const std::runtime_error& e) {
                        std::lock_guard<std::mutex> cerrLock(cerrMutex);
                        std::cerr << "Runtime error: " << e.what() << std::endl;
                    }
                    catch (const std::exception& e) {
                        std::lock_guard<std::mutex> cerrLock(cerrMutex);
                        std::cerr << "Exception: " << e.what() << std::endl;
                    }
                    catch (...) {
                        std::lock_guard<std::mutex> cerrLock(cerrMutex);
                        std::cerr << "Unknown error" << std::endl;
                    }
                    tryAgain = peerConnectPtr->Failed() && attempts < 3;
                } while (tryAgain);
            }
        );
    }

    {
        std::lock_guard<std::mutex> coutLock(coutMutex);
        std::cout << "Started " << peerThreads.size() << " threads for peers" << std::endl;
    }

    std::this_thread::sleep_for(30s);


    {
        std::lock_guard<std::mutex> coutLock(coutMutex);
        std::cout << "Terminating all peer connections" << std::endl;
    }
    for (auto& peerConnectPtr : peerConnections) {
        peerConnectPtr->Terminate();
    }

    for (std::thread& thread : peerThreads) {
        thread.join();
    }

    return false;
}

void DownloadTorrentFile(const TorrentFile& torrentFile, PieceStorage& pieces, const std::string& ourId) {
    std::cout << "Connecting to tracker " << torrentFile.announce << std::endl;
    TorrentTracker tracker(torrentFile.announce);
    bool requestMorePeers = false;
    do {
        tracker.UpdatePeers(torrentFile, ourId, 12345);

        if (tracker.GetPeers().empty()) {
            std::cerr << "No peers found. Cannot download a file" << std::endl;
        }

        std::cout << "Found " << tracker.GetPeers().size() << " peers" << std::endl;
        for (const Peer& peer : tracker.GetPeers()) {
            std::cout << "Found peer " << peer.ip << ":" << peer.port << std::endl;
        }

        requestMorePeers = RunDownloadMultithread(pieces, torrentFile, ourId, tracker);
    } while (requestMorePeers);
}

void TestTorrentFile(const fs::path& file, const fs::path& outputDirectory) {
    TorrentFile torrentFile;
    try {
        torrentFile = LoadTorrentFile(file);
        std::cout << "Loaded torrent file " << file << ". " << torrentFile.comment << std::endl;
    }
    catch (const std::invalid_argument& e) {
        std::cerr << e.what() << std::endl;
        return;
    }

    //const std::filesystem::path outputDirectory = PrepareDownloadDirectory(PeerId);
    PieceStorage pieces(torrentFile, outputDirectory);
    for (int i = 0; i < 1; ++i) {
        int ff = 0;
        DownloadTorrentFile(torrentFile, pieces, PeerId);

        CheckDownloadedPiecesIntegrity(outputDirectory / torrentFile.name, torrentFile, pieces);
        std::vector<size_t> pieceIndices = pieces.GetPiecesSavedToDiscIndices();
        std::vector<size_t> rez(torrentFile.pieceHashes.size(), 0);

        for (auto d : pieceIndices) {
            rez[d]++;
            if (d < 35) {
                std::cout << "no problem " << d << std::endl;
            }
        }
        for (int j = 0; j < 35; j++) {
            if (rez[j] == 0) {
                ff++;
                std::cout << "have problem " << j << std::endl;
                PiecePtr p = std::make_shared<Piece>(j, torrentFile.pieceLength, torrentFile.pieceHashes[j]);
                pieces.AddPieceInQueue(p);
            }
        }
        if (ff == 0) {
            break;
        }
    }

    std::cout << "Successfully downloaded the file" << std::endl;
}

int main(int argc, char* argv[]) {
    fs::path outputDirectory;
    int p;
    fs::path file;

    outputDirectory = argv[2];
    p = std::stoi(argv[4]);
    file = argv[5];

    try {
        TestTorrentFile(file, outputDirectory);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
