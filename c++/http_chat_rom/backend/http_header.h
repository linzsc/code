#include <cstdint>
#include <string>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <endian.h>

// 协议头结构（固定长度）
struct ProtocolHeader {


    // 魔数定义
    static const uint32_t MAGIC = 0x12345678;
    
    // 消息类型定义
    enum MsgType : uint16_t {
        HTTP_REQUEST = 1,      // HTTP 请求
        CHAT_MESSAGE ,      // 聊天消息
        LOGIN  ,             // 登录
        REGISTER ,           // 注册
        LOGOUT,              // 下线


        HTTP_RESPONSE = 100,   // HTTP 响应
        CHAT_RESPONSE,         // 聊天消息响应
        LOGIN_RESPONSE,        // 登录响应
        REGISTER_RESPONSE,     // 注册响应
        LOGOUT_RESPONSE,       // 下线响应
    };

    // 构造函数
    ProtocolHeader() : magic(MAGIC), version(1), msg_type(0), body_len(0) {}

    // 设置消息类型
    void setMessageType(MsgType type) {
        msg_type = static_cast<uint16_t>(type);
    }

    // 设置 Body 数据长度
    void setBodyLength(uint32_t len) {
        body_len = len;
    }

    // 序列化为字节流
    std::string serialize() const {
        // 将结构体数据打包成字节流
        char buffer[sizeof(ProtocolHeader)];
        memcpy(buffer, &magic, sizeof(magic));
        memcpy(buffer + sizeof(magic), &version, sizeof(version));
        memcpy(buffer + sizeof(magic) + sizeof(version), &msg_type, sizeof(msg_type));
        memcpy(buffer + sizeof(magic) + sizeof(version) + sizeof(msg_type), &body_len, sizeof(body_len));
        //printf("http_head_buffer:  %s\n",buffer);
        return std::string(buffer,sizeof(buffer));
    }

    // 从字节流中反序列化
    static ProtocolHeader deserialize(const std::string& data) {
        if (data.size() < sizeof(ProtocolHeader)) {
            throw std::invalid_argument("Invalid data size for ProtocolHeader");
        }

        ProtocolHeader header;
        const char* ptr = data.data();

        // 解包数据
        memcpy(&header.magic, ptr, sizeof(header.magic));
        ptr += sizeof(header.magic);
        memcpy(&header.version, ptr, sizeof(header.version));
        ptr += sizeof(header.version);
        memcpy(&header.msg_type, ptr, sizeof(header.msg_type));
        ptr += sizeof(header.msg_type);
        memcpy(&header.body_len, ptr, sizeof(header.body_len));

        // 验证魔数
        if (header.magic != MAGIC) {
            throw std::runtime_error("Invalid magic number in ProtocolHeader");
        }

        return header;
    }

    // 打印协议头信息
    void print() const {
        std::printf("ProtocolHeader:\n");
        std::printf("  Magic:      0x%X\n", magic);
        std::printf("  Version:    %d\n", version);
        std::printf("  Msg Type:   %d\n", msg_type);
        std::printf("  Body Len:   %d\n", body_len);
    }
    
    uint32_t magic;      // 魔数（标识协议）
    uint16_t version;    // 协议版本
    uint16_t msg_type;   // 消息类型
    uint32_t body_len;   // Body 数据长度（单位：字节）
};