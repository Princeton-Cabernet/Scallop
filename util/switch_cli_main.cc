
#include <boost/asio.hpp>
#include <csignal>
#include <cxxopts/cxxopts.h>
#include <iostream>

#include "../lib/switch_api_client.h"
#include "../lib/util.h"

namespace switch_cli {

    asio::io_context io;
    p4sfu::SwitchAPIClient client{io};

    struct args {
        struct {
            std::string ip = "127.0.0.1";
            unsigned short port = 4001;
        } config;
        std::vector<std::string> cmds;
    };

    cxxopts::Options setOptions() {

        cxxopts::Options opts("switch_cli", "P4-SFU Switch CLI Client");

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

        os << "  show,s\n"
           << "    streams,str\n"
           << "  set\n"
           << "    decode-target,dt <session-id> <ssrc> <participant-id> <decode-target>\n";

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

        args.cmds = parsed["cmd"].as<std::vector<std::string>>();

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

    auto args = switch_cli::parseOptions(switch_cli::setOptions(), argc, argv);

    signal(SIGTERM, switch_cli::signal_handler);
    signal(SIGINT, switch_cli::signal_handler);

    try {
        switch_cli::client.open(args.config.ip, args.config.port);
        switch_cli::client.runCLICommand(args.cmds);
        switch_cli::io.run();
        return 0;
    } catch (std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }
}
