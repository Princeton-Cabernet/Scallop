
#include <iostream>
#include <cxxopts/cxxopts.h>

#include "../lib/switch_agent.h"
#include "../lib/util.h"

void printHelp(cxxopts::Options& opts, int exitCode = 0) {

    std::ostream& os = (exitCode ? std::cerr : std::cout);
    os << opts.help({""}) << std::endl;
    exit(exitCode);
}

cxxopts::Options setOptions() {

    cxxopts::Options opts("model", "Switch SFU Model");

    opts.add_options()
        ("i,switch-id", "switch id", cxxopts::value<unsigned>(), "ID")
        ("l,sfu-listen-port", "SFU listen port", cxxopts::value<std::uint16_t>(),"PORT")
        ("x,api-listen-port", "API listen port", cxxopts::value<std::uint16_t>(),"PORT")
        ("c,controller", "controller address", cxxopts::value<std::string>(),"IP:PORT")
        ("u,ice-ufrag", "ICE user name fragment", cxxopts::value<std::string>(), "UFRAG")
        ("p,ice-pwd", "ICE password", cxxopts::value<std::string>(), "PWD")
        ("a,av1-rtp-ext", "RTP extension ID for AV1 dependency descriptor",
            cxxopts::value<unsigned>(), "ID")
        ("r,rtp-drop-rate", "RTP packet drop rate", cxxopts::value<double>(), "RATE")
        ("v,verbose", "log debug messages")
        ("h,help", "print this help message");

    return opts;
}

template <typename DataPlaneType>
typename p4sfu::SwitchAgent<DataPlaneType>::Config parseOptions(cxxopts::Options opts, int argc, char** argv) {

    // initialize default configuration:
    typename p4sfu::SwitchAgent<DataPlaneType>::Config config{
        .switchId       = 0,
        .sfuListenPort  = 3000,
        .apiListenPort  = 4001,
        .controllerIPv4 = "127.0.0.1",
        .controllerPort = 3302,
        .iceUfrag       = "7Ad/",
        .icePwd         = "UKZe/aYNEouGzQUhChnKGiIS",
        .av1RtpExtId    = 12,
        .rtpDropRate    = 0.0,
        .verbose        = false
    };

    auto parsed = opts.parse(argc, argv);

    if (parsed.count("i")) {
        config.switchId = parsed["i"].as<unsigned>();
    }

    if (parsed.count("l")) {
        config.sfuListenPort = parsed["l"].as<std::uint16_t>();
    }

    if (parsed.count("x")) {
        config.apiListenPort = parsed["x"].as<std::uint16_t>();
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

    if (parsed.count("a")) {
        config.av1RtpExtId = parsed["a"].as<unsigned>();
    }

    if (parsed.count("r")) {
        config.rtpDropRate = parsed["r"].as<double>();
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

    auto config = parseOptions<p4sfu::DataPlaneModel>(setOptions(), argc, argv);
    config.type = p4sfu::SwitchAgent<p4sfu::DataPlaneModel>::Config::Type::model;

    p4sfu::DataPlaneModel::Config dataPlaneConfig{
        .av1RtpExt   = config.av1RtpExtId,
        .port        = config.sfuListenPort,
        .rtpDropRate = config.rtpDropRate
    };

    try {
        p4sfu::SwitchAgent<p4sfu::DataPlaneModel> s(config, dataPlaneConfig);
        return s();
    } catch (std::exception& e) {
        std::cerr << "[ERROR] main: failed starting switch model: " << e.what() << std::endl;
        return -1;
    }
}
