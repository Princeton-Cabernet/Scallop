
#include <cxxopts/cxxopts.h>

#include "controller.h"

void printHelp(cxxopts::Options& opts, int exitCode = 0) {

    std::ostream& os = (exitCode ? std::cerr : std::cout);
    os << opts.help({""}) << std::endl;
    exit(exitCode);
}

cxxopts::Options setOptions() {

    cxxopts::Options opts("controller", "P4-SFU Controller");

    opts.add_options()
        ("c,client-side-port", "client-side listening port",
         cxxopts::value<std::uint16_t>(), "PORT")
        ("s,switch-side-port", "switch-side listening port",
         cxxopts::value<std::uint16_t>(), "PORT")
        ("d,data-plane-addr", "data-plane address (for signaling)",
         cxxopts::value<std::string>(), "IP:PORT")
        ("x,api-listen-port", "API listen port", cxxopts::value<std::uint16_t>(), "PORT")
        ("w,web-port", "web server port", cxxopts::value<std::uint16_t>(), "PORT")
        ("r,cert-file", "certificate file", cxxopts::value<std::string>(), "FILE")
        ("k,key-file", "key file", cxxopts::value<std::string>(), "FILE")
        ("l,limit-net", "limit subnet for data plane", cxxopts::value<std::string>(), "IP/MASK")
        ("v,verbose", "log debug messages")
        ("h,help", "print this help message");

    return opts;
}

p4sfu::Controller::Config parseOptions(cxxopts::Options opts, int argc, char** argv) {

    p4sfu::Controller::Config config{
        .clientSidePort = 3301,
        .switchSidePort = 3302,
        .dataPlaneIPv4  = "127.0.0.1",
        .dataPlanePort  = 3000,
        .apiListenPort  = 4000,
        .webPort        = 8000,
        .certFile       = "cert.pem",
        .keyFile        = "key.pem",
        .limitNetwork   = "0.0.0.0",
        .limitMask      = 0,
        .verbose        = false
    };

    auto parsed = opts.parse(argc, argv);

    if (parsed.count("c"))
        config.clientSidePort = parsed["c"].as<std::uint16_t>();

    if (parsed.count("s"))
        config.switchSidePort = parsed["s"].as<std::uint16_t>();

    if (parsed.count("d"))
        std::tie(config.dataPlaneIPv4 ,config.dataPlanePort)
            = util::parseIPPort(parsed["d"].as<std::string>());

    if (parsed.count("x"))
        config.apiListenPort = parsed["x"].as<std::uint16_t>();

    if (parsed.count("w"))
        config.webPort = parsed["w"].as<std::uint16_t>();

    if (parsed.count("r"))
        config.certFile = parsed["r"].as<std::string>();

    if (parsed.count("k"))
        config.keyFile = parsed["k"].as<std::string>();

    if (parsed.count("l"))
        std::tie(config.limitNetwork, config.limitMask)
            = util::parseCIDR(parsed["l"].as<std::string>());

    if (parsed.count("v"))
        config.verbose = true;

    if (parsed.count("h"))
        printHelp(opts);

    return config;
}

int main(int argc, char** argv) {

    auto config = parseOptions(setOptions(), argc, argv);

    p4sfu::Controller c{config};
    return c();
}
