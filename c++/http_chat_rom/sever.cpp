#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <sstream>
#include <openssl/sha.h>

#include <openssl/evp.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// 数据库模拟
std::unordered_map<std::string, std::string> user_db;
std::unordered_map<std::string, int> online_name;
std::unordered_map<int, std::string> online_fd;

// WebSocket 升级处理
std::string generate_accept_key(std::string key) {
    std::string guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = key + guid;

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.length(), hash);

    EVP_ENCODE_CTX *ctx = EVP_ENCODE_CTX_new();
    std::size_t out_len;
    std::string accept;

    unsigned char *encoded = new unsigned char[EVP_ENCODE_LENGTH(SHA_DIGEST_LENGTH)];
    EVP_EncodeInit(ctx);
    EVP_EncodeUpdate(ctx, encoded, &out_len, hash, SHA_DIGEST_LENGTH);
    EVP_EncodeFinal(ctx, encoded + out_len, &out_len);
    EVP_ENCODE_CTX_free(ctx);

    accept.assign(reinterpret_cast<char*>(encoded), out_len);
    delete[] encoded;

    size_t pos = accept.find('=');
    if (pos != std::string::npos) {
        accept.erase(pos);
    }

    return accept;
}

class Server {
private:
    net::io_context io_context;
    tcp::acceptor acceptor;
    std::map<int, std::string> online_clients;
    struct ClientContext {
        std::string buffer;
        bool is_websocket;
    };
    std::unordered_map<int, ClientContext> client_contexts;

public:
    Server(uint16_t port)
        : acceptor(io_context, tcp::endpoint(tcp::v4(), port)) {}

    void start() {
        accept();
        io_context.run();
    }

    void accept() {
        acceptor.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::cout << "New connection from: " << socket.remote_endpoint() << std::endl;
                handle_request(std::move(socket));
            }
            accept();
        });
    }

    void handle_request(tcp::socket socket) {
        beast::tcp_stream stream(std::move(socket));
        beast::flat_buffer buffer;
        http::request<http::string_body> request;

        http::async_read(stream, buffer, request, [this, &stream, &buffer, &request](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (ec) {
                return;
            }

            if (boost::iequals(request.target(), "/register")) {
                handle_register(stream, request);
            } else if (boost::iequals(request.target(), "/login")) {
                handle_login(stream, request);
            } else if (boost::iequals(request.target(), "/ws_upgrade")) {
                handle_websocket(stream, request, buffer);
            } else {
                handle_not_found(stream);
            }
        });
    }

    void handle_register(beast::tcp_stream& stream, http::request<http::string_body>& request) {
        // 注册逻辑
        std::istringstream iss(request.body());
        std::string username, password;
        std::string line;
        while (std::getline(iss, line)) {
            if (line.find("username=") != std::string::npos) {
                username = line.substr(line.find("=") + 1);
            } else if (line.find("password=") != std::string::npos) {
                password = line.substr(line.find("=") + 1);
            }
        }

        if (user_db.find(username) == user_db.end()) {
            user_db[username] = password;
            send_response(stream, 200, "Registration successful");
        } else {
            send_response(stream, 409, "Username already exists");
        }
    }

    void handle_login(beast::tcp_stream& stream, http::request<http::string_body>& request) {
        // 登录逻辑
        std::istringstream iss(request.body());
        std::string username, password;
        std::string line;
        while (std::getline(iss, line)) {
            if (line.find("username=") != std::string::npos) {
                username = line.substr(line.find("=") + 1);
            } else if (line.find("password=") != std::string::npos) {
                password = line.substr(line.find("=") + 1);
            }
        }

        if (user_db[username] == password) {
            online_name[username] = stream.socket().native_handle();
            online_fd[stream.socket().native_handle()] = username;
            send_response(stream, 200, "Login successful");
        } else {
            send_response(stream, 401, "Invalid credentials");
        }
    }

    void handle_websocket(beast::tcp_stream& stream, http::request<http::string_body>& request, beast::flat_buffer& buffer) {
        websocket::stream<beast::tcp_stream> ws(stream);
        ws.async_handshake(request, [this, &ws, &buffer](const boost::system::error_code& ec) {
            if (ec) {
                return;
            }

            beast::multi_buffer read_buffer;
            ws.async_read(read_buffer, [this, &ws, &read_buffer](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                if (ec) {
                    return;
                }

                std::string message = boost::beast::buffers_to_string(read_buffer.data());
                broadcast_message(message);

                ws.text(ws.got_text());
                ws.async_write(boost::asio::buffer(message), [this, &ws, &read_buffer](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                    if (ec) {
                        return;
                    }
                });
            });
        });
    }

    void handle_not_found(beast::tcp_stream& stream) {
        send_response(stream, 404, "Not Found");
    }

    void send_response(beast::tcp_stream& stream, int status_code, const std::string& message) {
        http::response<http::string_body> response;
        response.result(static_cast<http::status>(status_code));
        response.set(http::field::content_type, "text/plain");
        response.body() = message;
        response.prepare_payload();

        http::async_write(stream, response, [](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (ec) {
                std::cerr << "Failed to send response: " << ec.message() << std::endl;
            }
        });
    }

    void broadcast_message(const std::string& message) {
        for (auto& client : online_fd) {
            try {
                // Send message to each client
                auto& socket = client.first;
                boost::asio::write(socket, boost::asio::buffer(message));
            } catch (const std::exception& e) {
                std::cerr << "Error broadcasting message: " << e.what() << std::endl;
            }
        }
    }
};

int main() {
    try {
        Server server(12345);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}