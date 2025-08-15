
#include <boost/asio.hpp>
#include <csignal>
#include <cxxopts/cxxopts.h>
#include <iostream>

#include "../lib/controller_api_client.h"
#include "../lib/util.h"

namespace controller_cli {

    asio::io_context io;
    p4sfu::ControllerAPIClient client{io};

    struct args {
        struct {
            std::string ip = "127.0.0.1";
            unsigned short port = 4000;
        } config;
        std::vector<std::string> cmds;
    };

    cxxopts::Options setOptions() {

        cxxopts::Options opts("controller_cli", "P4-SFU Controller CLI Client");

        opts.add_options()
            ("cmd", "cli commands", cxxopts::value<std::vector<std::string>>())
            ("c,connect", "server address", cxxopts::value<std::string>(), "IP:PORT")
            ("h,help", "print this help message");

        opts.parse_positional({"cmd"});

        return opts;
    }

    void printHelp(cxxopts::Options& opts, int exitCode = 0) {

        std::ostream& os = (exitCode ? std::cerr : std::cout);
        os << opts.help({""}) << std::endl;
        exit(exitCode);
    }

    args parseOptions(cxxopts::Options opts, int argc, char** argv) {

        args args;

        auto parsed = opts.parse(argc, argv);

        if (parsed.count("c")) {
            std::tie(args.config.ip, args.config.port)
                    = util::parseIPPort(parsed["c"].as<std::string>());
        }

        if (parsed.count("h")) {
            printHelp(opts);
        }

        if (parsed.count("cmd")) {
            args.cmds = parsed["cmd"].as<std::vector<std::string>>();
        } else {
            args.cmds = {};
        }

        return args;
    }

    void signal_handler(int sig) {


        std::cout << "caught " << util::sigName(sig) << ", terminating..." << std::endl;
        /*
        switch_cli::client->close();
        io.stop();
        */
    }
}

int main(int argc, char** argv) {

    auto args = controller_cli::parseOptions(controller_cli::setOptions(), argc, argv);

    signal(SIGTERM, controller_cli::signal_handler);
    signal(SIGINT, controller_cli::signal_handler);

    try {
        controller_cli::client.open(args.config.ip, args.config.port);

        if (!args.cmds.empty()) {
            controller_cli::client.runCLICommand(args.cmds);
        } else {

            if (controller_cli::client.isConnected()) {
                std::cout << "connection successful, disconnecting..." << std::endl;
            }

            controller_cli::client.disconnect();
        }

        controller_cli::io.run();
        return 0;
    } catch (std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }
}
