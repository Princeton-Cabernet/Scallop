#!/usr/bin/env python3

portname_eport_map = {
    "veth0": 0,
    "veth2": 1,
    "veth4": 2,
    "veth6": 3,
    "veth8": 4,
    "veth10": 5,
    "veth12": 6,
    "veth14": 7,
    "veth16": 8,
    "veth18": 9,
    "veth20": 10,
    "veth22": 11,
    "veth24": 12,
    "veth26": 13,
    "veth28": 14,
    "veth30": 15,
    "veth32": 16,
    "veth250": 64
}

def eport(portname):
    return portname_eport_map[portname]