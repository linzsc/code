#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/system/error_code.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = net::ssl;               // from <boost/asio/ssl.hpp>
using tcp = net::ip::tcp;               // from <boost/asio/ip/tcp.hpp>

// Echoes messages back to the client
class WebSocketSession : public std::enable_shared_from_this<WebSocketSession>
{
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;

public:
    explicit WebSocketSession(tcp::socket socket) : ws_(std::move(socket)) {}

    // Start the WebSocket session
    void run()
    {
        // Set suggested timeout settings for the websocket
        ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        ws_.set_option(websocket::stream_base::decorator([](websocket::response_type& res) {
            res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-sync");
        }));

        // Accept the WebSocket handshake
        ws_.accept(req_);

        // Run the WebSocket loop
        do_read();
    }

    void do_read()
    {
        // Read a message into our buffer
        ws_.async_read(buffer_,
            beast::bind_front_handler(&WebSocketSession::on_read, shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This indicates that the session was closed
        if (ec == websocket::error::closed)
            return;

        if (ec)
            return fail(ec, "read");

        // Echo the message back
        do_write();
    }

    void do_write()
    {
        ws_.async_write(buffer_.data(),
            beast::bind_front_handler(&WebSocketSession::on_write, shared_from_this()));
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        // Clear the buffer
        buffer_.consume(buffer_.size());

        // Do another read
        do_read();
    }

    void fail(beast::error_code ec, char const* what)
    {
        std::cerr << "Error on " << what << ": " << ec.message() << "\n";
    }
};

// Accepts incoming connections and launches the sessions
class Listener : public std::enable_shared_from_this<Listener>
{
    net::io_context& ioc_;
    ssl::context& ctx_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;

public:
    Listener(net::io_context& ioc, ssl::context& ctx, unsigned short port)
        : ioc_(ioc), ctx_(ctx), acceptor_(ioc, tcp::endpoint(tcp::v4(), port)), socket_(ioc)
    {
        // Start accepting connections
        do_accept();
    }

    // Start accepting connections
    void run()
    {
        do_accept();
    }

    void do_accept()
    {
        acceptor_.async_accept(socket_,
            beast::bind_front_handler(&Listener::on_accept, shared_from_this()));
    }

    void on_accept(beast::error_code ec)
    {
        if (ec)
        {
            std::cerr << "Error on accept: " << ec.message() << "\n";
        }
        else
        {
            // Create the WebSocket session and run it
            std::make_shared<WebSocketSession>(std::move(socket_))->run();
        }

        // Accept the next connection
        do_accept();
    }
};

int main(int argc, char* argv[])
{
    try
    {
        // Check command line arguments
        if (argc != 2)
        {
            std::cerr << "Usage: websocket-server-sync <port>\n";
            return EXIT_FAILURE;
        }

        // Initialize Asio and SSL
        net::io_context ioc;
        ssl::context ctx(ssl::context::tlsv12);

        // Load the server certificate and private key
        ctx.use_certificate_chain_file("server.pem");
        ctx.use_private_key_file("server.pem", ssl::context::pem);

        // Create and launch the listener
        std::make_shared<Listener>(ioc, ctx, std::atoi(argv[1]))->run();

        // Run the I/O service on the main thread
        ioc.run();
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error occurred: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}