#include <hiredis/hiredis.h>
#include <iostream>

int main() {
    redisContext* c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        std::cerr << "连接失败: " << c->errstr << std::endl;
        return 1;
    }

    redisReply* reply = (redisReply*)redisCommand(c, "SET key %s", "value");
    std::cout << "SET命令返回: " << reply->str << std::endl;
    freeReplyObject(reply);

    reply = (redisReply*)redisCommand(c, "GET key");
    std::cout << "GET命令返回: " << reply->str << std::endl;
    freeReplyObject(reply);

    redisFree(c);
    return 0;
}