#include <iostream>
#include <stdint.h>
#include <string>
// 协议Header（固定长度，网络字节序）
struct ProtocolHeader {
    uint32_t magic;     // 魔数（标识协议，如0x12345678）
    uint16_t version;   // 协议版本（如1）
    uint16_t msg_type;  // 消息类型（如1-HTTP请求，2-聊天消息）
    uint32_t body_len;  // Body数据长度（单位：字节）
    std::string body;    // Body数据（可变长度，JSON格式）
};

// 协议Body（可变长度，JSON格式）
// 例如聊天消息的Body：
/*
{
    "user": "Alice",
    "content": "Hello!",
    "timestamp": 1718000000
}
*/