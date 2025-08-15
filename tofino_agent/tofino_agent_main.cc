
#include <iostream>
#include <cxxopts/cxxopts.h>

#include "../lib/tofino_data_plane.h"
#include "../lib/switch_agent.h"

void printHelp(cxxopts::Options& opts, int exitCode = 0) {

    std::ostream& os = (exitCode ? std::cerr : std::cout);
    os << opts.help({""}) << std::endl;
    exit(exitCode);
}

cxxopts::Options setOptions() {

    cxxopts::Options opts("tofino_agent", "Scallop Tofino Agent");

    opts.add_options()
        ("i,switch-id", "switch id", cxxopts::value<unsigned>(), "ID")
        ("l,data-plane-iface", "data-plane interface (required)", cxxopts::value<std::string>(), "IFACE")
        ("c,controller", "controller address", cxxopts::value<std::string>(),"IP:PORT")
        ("u,ice-ufrag", "ICE user-name fragment", cxxopts::value<std::string>(), "UFRAG")
        ("p,ice-pwd", "ICE password", cxxopts::value<std::string>(), "PWD")
        ("x,api-listen-port", "API listen port", cxxopts::value<std::uint16_t>(), "PORT")
        ("v,verbose", "log debug messages")
        ("h,help", "print this help message");

    return opts;
}

template <typename DataPlaneType>
typename p4sfu::SwitchAgent<DataPlaneType>::Config parseOptions(cxxopts::Options opts, int argc, char** argv) {

    typename p4sfu::SwitchAgent<DataPlaneType>::Config config{
        .switchId       = 0,
        .apiListenPort  = 4001,
        .controllerIPv4 = "127.0.0.1",
        .controllerPort = 3302,
        .iceUfrag       = "7Ad/",
        .icePwd         = "UKZe/aYNEouGzQUhChnKGiIS",
        .verbose        = false
    };

    auto parsed = opts.parse(argc, argv);

    if (parsed.count("i")) {
        config.switchId = parsed["i"].as<unsigned>();
    }

    if (parsed.count("l")) {
        config.dataPlaneIface = parsed["l"].as<std::string>();
    } else {
        printHelp(opts, 1);
    }

    if (parsed.count("c")) {
        std::tie(config.controllerIPv4 ,config.controllerPort)
                = util::parseIPPort(parsed["c"].as<std::string>());
    }

    if (parsed.count("u")) {
        config.iceUfrag = parsed["u"].as<std::string>();
    }

    if (parsed.count("p")) {
        config.icePwd = parsed["p"].as<std::string>();
    }

    if (parsed.count("x")) {
        config.apiListenPort = parsed["x"].as<std::uint16_t>();
    }

    if (parsed.count("v")) {
        config.verbose = true;
    }

    if (parsed.count("h")) {
        printHelp(opts);
    }

    return config;
}

int main(int argc, char** argv) {

    auto config = parseOptions<p4sfu::TofinoDataPlane>(setOptions(), argc, argv);

    config.type = p4sfu::SwitchAgent<p4sfu::TofinoDataPlane>::Config::Type::tofino;

    p4sfu::TofinoDataPlane::Config dataPlaneConfig{
        .verbose = config.verbose,
        .tofinoListenPort = 8765,
        .dataPlaneIface = config.dataPlaneIface
    };

    try {
        p4sfu::SwitchAgent<p4sfu::TofinoDataPlane> s(config, dataPlaneConfig);
        return s();
    } catch (std::exception& e) {
        std::cerr << "failed initializing Tofino agent: " << e.what() << std::endl;
        return -1;
    }
}
