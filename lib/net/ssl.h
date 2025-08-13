
#ifndef SSL_H
#define SSL_H

#include <stdexcept>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using namespace boost;

namespace ssl {

    asio::ssl::context initializeSSL(const std::string& certFile, const std::string& keyFile) {

        asio::ssl::context ssl{asio::ssl::context::tlsv12};

        ssl.set_options(asio::ssl::context::default_workarounds |
                        asio::ssl::context::no_sslv2 |
                        asio::ssl::context::single_dh_use);

        boost::system::error_code ec;

        ec = ssl.use_certificate_chain_file(certFile, ec);

        if (ec) {
            throw std::runtime_error{"use_certificate_chain_file: failed: " + ec.message()};
        }

        ec = ssl.use_private_key_file(keyFile, boost::asio::ssl::context::file_format::pem, ec);

        if (ec) {
            throw std::runtime_error{"use_private_key_file: failed: " + ec.message()};
        }

        return ssl;
    }
}

#endif