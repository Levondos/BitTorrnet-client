#include "message.h"
#include "byte_tools.h"
#include <stdexcept>

Message Message::Parse(const std::string& messageString){
    Message msg;
    msg.messageLength = messageString.size();
    if (msg.messageLength == 0){
        msg.id = MessageId::KeepAlive;
        return msg;
    }
    uint8_t id = messageString[0];
    if (9 < id){
        throw std::invalid_argument("Wrong file format: Invalid message ID: " + std::to_string(id) + " is out of range [0, 9]");
    }
    msg.id = static_cast<MessageId>(id);
    msg.payload = messageString.substr(1, msg.messageLength-1);
    return msg;
}

Message Message::Init(MessageId id, const std::string& payload = ""){
    uint8_t num_id = static_cast<uint8_t>(id);
    if (9 < num_id){
        throw std::invalid_argument("Wrong id > 9");
    }
    if (num_id <= 3 && !payload.empty()){
        throw std::invalid_argument("Such id doesn't support payload transmission");
    }

    Message msg;
    msg.id = id;
    msg.messageLength = 1+payload.size();
    msg.payload = payload;
    return msg;
}

std::string Message::ToString() const{
    if (id == MessageId::KeepAlive){
        return IntToBytes(0); //возвращает 4 нулевых байта
    }

    std::string coded_message;
    coded_message.append(IntToBytes(messageLength));
    coded_message.push_back(static_cast<char>(id));
    coded_message.append(payload);
    return coded_message;
}

