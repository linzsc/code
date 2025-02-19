#ifndef HTTP_HEADER_H
#define HTTP_HEADER_H

#include <sstream>
#include <string>
#include <regex>
#include <map>
#include <unordered_map>
enum class HttpStatus {
    OK = 200,
    NOT_FOUND = 404,
    SWITCHING_PROTOCOLS = 101,
};

enum class HttpMethod {
    UNKNOWN = -1,
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    PATCH,
    TRACE,
    CONNECT,
};

struct HttpRequest {
    HttpMethod method;
    std::string path;
    std::string http_version;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    HttpStatus status;
    std::string http_version;
    std::map<std::string, std::string> headers;
    std::string body;
};

class HttpParser {
public:
    static HttpRequest parseHttpRequest(const std::string& request) {
        HttpRequest httpRequest;
        size_t end_header = request.find("\r\n\r\n");
        
        if (end_header != std::string::npos) {
            std::string request_line = request.substr(0, request.find("\r\n"));
            std::string headers_str = request.substr(request.find("\r\n") + 2, end_header - request.find("\r\n") - 2);
            httpRequest.body = request.substr(end_header + 4);
            
            std::regex request_regex("^(GET|POST|PUT|DELETE|HEAD|OPTIONS|PATCH|TRACE|CONNECT) (\\S+) (HTTP/1\\.\\d)");
            std::smatch match;
            if (std::regex_search(request_line, match, request_regex)) {
                httpRequest.method = strToHttpMethod(match[1]);
                httpRequest.path = match[2];
                httpRequest.http_version = match[3];
            }
            
            size_t header_start = 0;
            while (header_start < headers_str.size()) {
                size_t header_end = headers_str.find("\r\n", header_start);
                if (header_end == std::string::npos) break;
                std::string header = headers_str.substr(header_start, header_end - header_start);
                size_t colon_pos = header.find(": ");
                if (colon_pos != std::string::npos) {
                    httpRequest.headers[header.substr(0, colon_pos)] = header.substr(colon_pos + 2);
                }
                header_start = header_end + 2;
            }
        }
        return httpRequest;
    }
    
    static HttpResponse parseHttpResponse(const std::string& response) {
        HttpResponse httpResponse;
        size_t end_header = response.find("\r\n\r\n");
        
        if (end_header != std::string::npos) {
            std::string status_line = response.substr(0, response.find("\r\n"));
            std::string headers_str = response.substr(response.find("\r\n") + 2, end_header - response.find("\r\n") - 2);
            httpResponse.body = response.substr(end_header + 4);
            
            std::regex status_regex("^(HTTP/1\\.\\d) (\\d{3})");
            std::smatch match;
            if (std::regex_search(status_line, match, status_regex)) {
                httpResponse.http_version = match[1];
                httpResponse.status = static_cast<HttpStatus>(std::stoi(match[2]));
            }
            
            size_t header_start = 0;
            while (header_start < headers_str.size()) {
                size_t header_end = headers_str.find("\r\n", header_start);
                if (header_end == std::string::npos) break;
                std::string header = headers_str.substr(header_start, header_end - header_start);
                size_t colon_pos = header.find(": ");
                if (colon_pos != std::string::npos) {
                    httpResponse.headers[header.substr(0, colon_pos)] = header.substr(colon_pos + 2);
                }
                header_start = header_end + 2;
            }
        }
        return httpResponse;
    }
    
    static std::string createHttpResponse(HttpStatus status, const std::string& content_type, const std::string& body) {
        std::ostringstream response_stream;
        response_stream << "HTTP/1.1 " << static_cast<int>(status) << " \r\n";
        response_stream << "Content-Type: " << content_type << "\r\n";
        response_stream << "Content-Length: " << body.length() << "\r\n\r\n";
        response_stream << body;
        return response_stream.str();
    }
    
    static void printHttpRequest(const HttpRequest& request) {
        std::cout << "HTTP Request:\n";
        std::cout << "Method: " << static_cast<int>(request.method) << "\n";
        std::cout << "Path: " << request.path << "\n";
        std::cout << "Version: " << request.http_version << "\n";
        std::cout << "Headers:\n";
        for (const auto& header : request.headers) {
            std::cout << "  " << header.first << ": " << header.second << "\n";
        }
        std::cout << "Body:\n" << request.body << "\n";
    }

    static void printHttpResponse(const HttpResponse& response) {
        std::cout << "HTTP Response:\n";
        std::cout << "Version: " << response.http_version << "\n";
        std::cout << "Status: " << static_cast<int>(response.status) << "\n";
        std::cout << "Headers:\n";
        for (const auto& header : response.headers) {
            std::cout << "  " << header.first << ": " << header.second << "\n";
        }
        std::cout << "Body:\n" << response.body << "\n";
    }
private:
    static HttpMethod strToHttpMethod(const std::string& method) {
        static const std::unordered_map<std::string, HttpMethod> method_map = {
            {"GET", HttpMethod::GET}, {"POST", HttpMethod::POST}, {"PUT", HttpMethod::PUT},
            {"DELETE", HttpMethod::DELETE}, {"HEAD", HttpMethod::HEAD}, {"OPTIONS", HttpMethod::OPTIONS},
            {"PATCH", HttpMethod::PATCH}, {"TRACE", HttpMethod::TRACE}, {"CONNECT", HttpMethod::CONNECT}
        };
        auto it = method_map.find(method);
        return it != method_map.end() ? it->second : HttpMethod::UNKNOWN;
    }
};

#endif // HTTP_HEADER_H
