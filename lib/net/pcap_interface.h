#ifndef PCAP_INTERFACE_H
#define PCAP_INTERFACE_H

#include <pcap.h>
#include "pcap_util.h"

class PCAPInterface {

public:
    explicit PCAPInterface(const std::string& iface_name) {

        _pcap = pcap_open_live(iface_name.c_str(), 2048, false, 1, _errbuf);

        if (!_pcap) {
            throw std::runtime_error("PCAPInterface: failed pcap_open_live " + iface_name);
        }
    }

    [[nodiscard]] int fd() {

        auto fd = pcap_get_selectable_fd(_pcap);

        if (fd == PCAP_ERROR) {
            throw std::runtime_error("PCAPInterface: failed get_selectable_fd");
        }

        return fd;
    }

    bool next(pcap_pkt& pkt) {

        if (pcap_next_ex(_pcap, &_hdr, &_plBuf) == 1) {
            pkt.buf       = _plBuf;
            pkt.ts        = _hdr->ts;
            pkt.frame_len = _hdr->len;
            pkt.cap_len   = _hdr->caplen;
            return true;
        }

        return false;
    }

    unsigned inject(const unsigned char* buf, unsigned len) {

        return pcap_inject(_pcap, buf, len);
    }

    void close() {
        pcap_close(_pcap);
    }

    ~PCAPInterface() = default;

private:
    pcap* _pcap = nullptr;
    struct pcap_pkthdr* _hdr = {};
    const u_char* _plBuf = {};
    char _errbuf[PCAP_ERRBUF_SIZE] = {};
};

#endif
