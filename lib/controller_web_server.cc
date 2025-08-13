
#include "controller_web_server.h"

#include "util.h"

p4sfu::ControllerWebServer::ControllerWebServer(boost::asio::io_context& io, std::uint16_t port,
                                                boost::asio::ssl::context& ssl)
    : _server{io, port, ssl} {

    _server.serveDirectory("client/dist");

    _server.addRoute("/", [this](HTTPServer::Session& s, const HTTPServer::Target& target) {
        this->_serveSFUApp(s, target);
    });
}

void p4sfu::ControllerWebServer::_serveSFUApp(HTTPServer::Session& s,
                                              const HTTPServer::Target& target) {

    http::response<http::string_body> response{http::status::ok, s.request().version()};
    response.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(http::field::content_type, "text/html");
    response.keep_alive(s.request().keep_alive());

    auto tpl = _inja.parse(util::readFile("client/views/client.html.inja"));

    // parse GET parameters:

    // set default config:
    json::json config = {
        {"sessionId", 1}
    };

    // session configuration:
    if (target.params().contains("s")) {
        config["sessionId"] = std::stoul(target.params().at("s"));
    }

    // controller configuration:
    if (target.params().contains("c")) {
        auto ipPort = util::parseIPPort(target.params().at("c"));
        config["controller"] = {
            {"host", std::get<0>(ipPort)},
            {"port", std::get<1>(ipPort)}
        };
    }

    response.body() = _inja.render(tpl, {
        {"config",             util::escapeJSONForHTML(config.dump())},
        {"sender-tpl",         util::readFile("client/views/sender-template.mustache")},
        {"receiver-panel-tpl", util::readFile("client/views/receiver-panel-tpl.mustache")},
        {"receiver-info-tpl",  util::readFile("client/views/receiver-info-tpl.mustache")},
    });

    response.prepare_payload();
    http::write(s.stream(), response);
}
