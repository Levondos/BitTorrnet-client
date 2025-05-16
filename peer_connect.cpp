#include "byte_tools.h"
#include "peer_connect.h"
#include "message.h"
#include <iostream>
#include <sstream>
#include <utility>
#include <cassert>

using namespace std::chrono_literals;

/*class PeerPiecesAvailability------------------------------------------------------------*/
PeerPiecesAvailability::PeerPiecesAvailability(){}

PeerPiecesAvailability::PeerPiecesAvailability(std::string bitfield): 
    bitfield_{bitfield}
    {}

bool PeerPiecesAvailability::IsPieceAvailable(size_t pieceIndex) const{
    if (pieceIndex >= Size()){
        throw std::invalid_argument("There is no such bit in the bitfield");
    }

    size_t pos_byte = bitfield_.size()-1 - pieceIndex/8;
    size_t pos_bit = 7 - pieceIndex%8;

    return (bitfield_[pos_byte] & static_cast<uint8_t>(1 << pos_bit)) != 0;
}

void PeerPiecesAvailability::SetPieceAvailability(size_t pieceIndex){
    if (pieceIndex >= Size()){
        throw std::invalid_argument("There is no such bit in the bitfield");
    }

    size_t pos_byte = bitfield_.size()-1 - pieceIndex/8;
    size_t pos_bit = 7 - pieceIndex%8;

    bitfield_[pos_byte] |= static_cast<uint8_t>(1 << pos_bit);
}

size_t PeerPiecesAvailability::Size() const{
    return 8 * bitfield_.size();
}
/*----------------------------------------------------------------------------------------*/

/*class PeerConnect-----------------------------------------------------------------------*/
PeerConnect::PeerConnect(const Peer& peer, const TorrentFile& tf, std::string selfPeerId):
    tf_(tf),
    socket_(peer.ip, peer.port, std::chrono::seconds(2), std::chrono::seconds(2)),
    selfPeerId_(std::move(selfPeerId)),
    peerId_(""),
    piecesAvailability_(),
    terminated_(false),
    choked_(true)
{
    if (selfPeerId_.size() != 20){
        throw std::invalid_argument("'selfPeerId_' length should be 20");
    }
}

void PeerConnect::Run() {
    while (!terminated_) {
        if (EstablishConnection()) {
            std::cout << "Connection established to peer" << std::endl;
            MainLoop();
        } else {
            std::cerr << "Cannot establish connection to peer" << std::endl;
            Terminate();
        }
    }
}

void PeerConnect::PerformHandshake() {
    // Производим подключение
    socket_.EstablishConnection();

    std::string handshake;
    handshake += static_cast<char>(19);
    handshake += "BitTorrent protocol";
    handshake += std::string(8, '\0');//никаких дополнительных флагов не выбираем
    handshake += tf_.infoHash;
    handshake += selfPeerId_;

    socket_.SendData(handshake);

    std::string response = socket_.ReceiveData(68);
    if (response.size() != 68) {
        throw std::runtime_error("Invalid handshake response length");
    }
    if (response.substr(0, 20) != handshake.substr(0, 20)) {
        throw std::runtime_error("Protocol mismatch in handshake");
    }
    if (response.substr(28, 20) != tf_.infoHash) {
        throw std::runtime_error("Info hash mismatch in handshake");
    }

    peerId_ = response.substr(48, 20);
}

bool PeerConnect::EstablishConnection() {
    try {
        PerformHandshake();
        ReceiveBitfield();
        SendInterested();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to establish connection with peer " << socket_.GetIp() << ":" <<
            socket_.GetPort() << " -- " << e.what() << std::endl;
        return false;
    }
}

void PeerConnect::ReceiveBitfield() {
    bool was_bitfield = false;
    bool was_have = false;
    while (choked_){
        Message msg = Message::Parse(socket_.ReceiveData(0));

        if (msg.id == MessageId::BitField) {
            //согласно стандарту: другой BitField и Have не могут быть перед BitField
            if (was_bitfield){
                throw std::runtime_error("BitField: BitField has been received earlier");
            }
            if (was_have){
                throw std::runtime_error("BitField: Have has been received earlier, \
                                        BitField haven't been expected");
            }
            was_bitfield = true;

            //проверка на длину
            size_t numPieces = tf_.pieceHashes.size();
            size_t expectedBitfieldSize = (numPieces + 7) / 8;
            if (msg.payload.size() != expectedBitfieldSize) {
                throw std::runtime_error("BitField: Invalid bitfield size - expected " + 
                                        std::to_string(expectedBitfieldSize) +
                                        ", got " +
                                        std::to_string(msg.payload.size()));
            }

            //проверка запасных битов (они должны быть равны 0)
            size_t spareBits = expectedBitfieldSize*8 - numPieces;
            if (spareBits != 0){
                uint8_t last_byte = static_cast<uint8_t>(msg.payload.back());
                for (int i = 0; i < spareBits; ++i){
                    if (last_byte % 2 == 1){
                        throw std::runtime_error("BitField: spare bits in bitfield are not zero");
                    }
                    last_byte >>= 1;
                }
            }

            piecesAvailability_ = PeerPiecesAvailability(msg.payload);
        } 
        else if (msg.id == MessageId::Have){
            was_have = true;
            if (msg.payload.size() != 4){
                throw std::runtime_error("Have: size payload isn't 4");
            }

            int32_t index = BytesToInt(msg.payload);
            if (index >= piecesAvailability_.Size()){
                throw std::runtime_error("Have: wrong index >= count pieces");
            }

            piecesAvailability_.SetPieceAvailability(index);
        }
        else if (msg.id == MessageId::Unchoke) {
            choked_ = false;
        } 
        else if (msg.id == MessageId::KeepAlive){
            continue;
        }
        else {
            throw std::runtime_error("Unexpected message type: " + std::to_string(static_cast<int>(msg.id)));
        }
    }
}

void PeerConnect::SendInterested(){
    Message interestedMsg = Message::Init(MessageId::Interested, "");
    std::string msgStr = interestedMsg.ToString();
    socket_.SendData(msgStr);
}

void PeerConnect::Terminate() {
    std::cerr << "Terminate" << std::endl;
    socket_.CloseConnection();
    terminated_ = true;
}

void PeerConnect::MainLoop() {
    /*
     * При проверке вашего решения на сервере этот метод будет переопределен.
     * Если вы провели handshake верно, то в этой функции будет работать обмен данными с пиром
     */
    std::cout << "Dummy main loop" << std::endl;
    Terminate();
}
/*---------------------------------------------------------------------------------------*/
