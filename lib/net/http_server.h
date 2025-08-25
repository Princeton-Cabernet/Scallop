
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>

#include <filesystem>
#include <iostream>
#include <map>
#include <optional>

using namespace boost;
using namespace boost::beast;

class HTTPServer {

public:

    class Target;
    class Session;

    typedef std::function<void(Session&, const HTTPServer::Target&)> HandlerFx;

    class Target {

    public:

        explicit Target(const std::string& target) {
            _parseURL(target);
        }

        [[nodiscard]] const std::string& path() const {
            return _path;
        }

        [[nodiscard]] const std::string& fileExtension() const {
            return _fileExtension;
        }

        [[nodiscard]] const std::map<std::string, std::string>& params() const {
            return _params;
        }

    private:

        static std::string _decodeURL(const std::string& str) {

            std::string decoded;
            char ch;
            unsigned int i, ii;

            for (i = 0; i < str.length(); i++) {

                if (str[i] == '%') {
                    ii = std::strtoul(str.substr(i + 1, 2).c_str(), nullptr, 16);
                    ch = static_cast<char>(ii);
                    decoded += ch;
                    i = i + 2;
                } else if (str[i] == '+') {
                    decoded += ' ';
                } else {
                    decoded += str[i];
                }
            }

            return decoded;
        }

        void _parseURL(const std::string &url) {

            std::map<std::string, std::string> params;
            std::string path, file_extension;

            // Find the position of the query part (`?`)
            std::string::size_type pos = url.find('?');
            std::string path_and_file = (pos == std::string::npos) ? url : url.substr(0, pos);
            std::string query = (pos == std::string::npos) ? "" : url.substr(pos + 1);

            // Extract file extension, if any, from the path
            std::string::size_type ext_pos = path_and_file.rfind('.');

            if (ext_pos != std::string::npos && ext_pos > path_and_file.rfind('/')) {
                _fileExtension = path_and_file.substr(ext_pos + 1);
            }

            _path = path_and_file;

            // Split the query string into key-value pairs
            std::istringstream query_stream(query);
            std::string pair;

            while (std::getline(query_stream, pair, '&')) {

                std::string::size_type equals_pos = pair.find('=');

                if (equals_pos != std::string::npos) {
                    std::string key = _decodeURL(pair.substr(0, equals_pos));
                    std::string value = _decodeURL(pair.substr(equals_pos + 1));
                    _params[key] = value;
                } else {
                    // Handle case where there's a key with no value
                    std::string key = _decodeURL(pair);
                    _params[key] = "";
                }
            }
        }

        std::string _path;
        std::string _fileExtension;
        std::map<std::string, std::string> _params;
    };

    explicit HTTPServer(asio::io_context& io, unsigned short port, asio::ssl::context& ssl)
            : _acceptor{io, asio::ip::tcp::endpoint{asio::ip::tcp::v4(), port}},
              _socket{io},
              _ssl{ssl} {

        _configureMimeTypes();
        _accept();
    }

    void serveDirectory(const std::string& docRoot) {
        _docRoot = docRoot;
    }

    class Session : public std::enable_shared_from_this<Session> {
    public:
        explicit Session(HTTPServer& server, asio::ip::tcp::socket socket, asio::ssl::context& ssl)
                : _server{server},
                  _stream{std::move(socket), ssl} { }

        void start() {
            _stream.async_handshake(asio::ssl::stream_base::server,
                [this, self = shared_from_this()](boost::beast::error_code ec) {

                if (!ec) {
                    self->_read();
                }
            });
        }

        const http::request<http::dynamic_body>& request() const {
            return _request;
        }

        asio::ssl::stream<asio::ip::tcp::socket>& stream() {
            return _stream;
        }

    private:

        void _read() {

            http::async_read(_stream, _read_buf, _request,
                 [this, self = this->shared_from_this()]
                     (boost::system::error_code ec, std::size_t len) {

                 _handleRequest();
            });
        }

        void _handleRequest() {

            if (_request.target().empty()) {
                std::cout << "HTTPServer: _handleRequest(): empty target - return 400" << std::endl;
                _returnStatus(http::status::bad_request);
                return;
            }

            if (_request.method() != http::verb::get) {
                std::cout << "HTTPServer: _handleRequest(): non-get request - return 405" << std::endl;
                _returnStatus(http::status::method_not_allowed);
                return;
            }

            Target target{std::string{_request.target()}};

            boost::beast::error_code ec;

            if (_server._routes.find(target.path()) != _server._routes.end()) {
                std::cout << "HTTPServer: _handleRequest(): resource found " << target.path() << std::endl;
                _server._routes[target.path()](*this, target);
                return;
            }

            if (_server._docRoot) {

                auto filePath = _server._docRoot.value() + target.path();

                if (std::filesystem::exists(filePath)) {

                    if (std::filesystem::is_directory(filePath)) {

                        if (std::filesystem::exists(filePath + "/index.html")) {
                            filePath += "/index.html";
                        } else {
                            _returnStatus(http::status::not_found);
                            return;
                        }
                    }

                    beast::http::file_body::value_type body;

                    body.open(filePath.c_str(), beast::file_mode::scan, ec);

                    if (ec) {
                        _returnStatus(http::status::internal_server_error);
                        return;
                    }

                    http::response<http::file_body> response{
                            std::piecewise_construct, std::make_tuple(std::move(body)),
                            std::make_tuple(http::status::ok, _request.version())};

                    response.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                    response.set(http::field::content_type,
                                 _server._getMimeType(target.fileExtension()));
                    http::write(_stream, response, ec);

                    if (ec) {
                        _returnStatus(http::status::internal_server_error);
                    }

                    return;
                }
            }

            _returnStatus(http::status::not_found);
        }

        void _returnStatus(http::status status) {

            http::response<http::string_body> response{status, _request.version()};
            response.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            response.set(http::field::content_type, "text/html");
            response.keep_alive(_request.keep_alive());

            switch (status) {
                case http::status::ok:
                    response.body() = "200 OK";
                    break;
                case http::status::not_found:
                    response.body() = "404 Not Found";
                    break;
                case http::status::internal_server_error:
                    response.body() = "500 Internal Server Error";
                    break;
                default:
                    response.body() = "Unknown status";
                    break;
            }

            response.prepare_payload();

            boost::beast::error_code ec;

            http::write(_stream, response, ec);

            if (ec) {
                std::cout << "HTTPServer: _returnStatus(): write failed: " << ec.message() << std::endl;
            }
        }

        HTTPServer& _server;
        asio::ssl::stream<asio::ip::tcp::socket> _stream;
        beast::flat_buffer _read_buf;
        http::request<http::dynamic_body> _request;
    };

    void addRoute(const std::string& path, HandlerFx handler) {

        _routes[path] = std::move(handler);
    }

private:

    void _accept() {
        _acceptor.async_accept(_socket, [this](std::error_code ec) {
            std::make_shared<Session>(*this, std::move(_socket), _ssl)->start();
            _accept();
        });
    }

    void _configureMimeTypes() {

        _mimeTypes["css"]  = "text/css";
        _mimeTypes["gif"]  = "image/gif";
        _mimeTypes["html"] = "text/html";
        _mimeTypes["ico"]  = "image/vnd.microsoft.icon";
        _mimeTypes["jpeg"] = "image/jpeg";
        _mimeTypes["jpg"]  = "image/jpeg";
        _mimeTypes["js"]   = "application/javascript";
        _mimeTypes["json"] = "application/json";
        _mimeTypes["map"]  = "application/json";
        _mimeTypes["png"]  = "image/png";
        _mimeTypes["svg"]  = "image/svg+xml";
        _mimeTypes["txt"]  = "text/plain";
    }

    std::string _getMimeType(const std::string& extension) {

        auto mimeType = _mimeTypes.find(extension);
        return (mimeType != _mimeTypes.end()) ? mimeType->second : DEFAULT_MIME_TYPE;
    }

    asio::ip::tcp::acceptor _acceptor;
    asio::ip::tcp::socket _socket;
    asio::ssl::context& _ssl;
    std::optional<std::string> _docRoot;
    std::unordered_map<std::string, HandlerFx> _routes;
    std::unordered_map<std::string, std::string> _mimeTypes;
    const std::string DEFAULT_MIME_TYPE = "application/octet-stream";
};

#endif
