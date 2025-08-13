#ifndef P4_SFU_STUN_H
#define P4_SFU_STUN_H

#include <cstdint>
#include <cstring>

extern "C" {
#include <stun/stunagent.h>
#include <stun/usages/ice.h>
}


#include "../util.h"

namespace stun {

    struct Credentials {
        std::string uFrag;
        std::string pwd;
        bool validated = false;
    };

    struct ValidationResult {
        bool success = false;
        bool initial = false;
        std::string message;
    };

    static std::string validationResultString(const StunValidationStatus& s) {

        switch (s) {
            case STUN_VALIDATION_SUCCESS:
                return "STUN_VALIDATION_SUCCESS";
            case STUN_VALIDATION_NOT_STUN:
                return "STUN_VALIDATION_NOT_STUN";
            case STUN_VALIDATION_INCOMPLETE_STUN:
                return "STUN_VALIDATION_INCOMPLETE_STUN";
            case STUN_VALIDATION_BAD_REQUEST:
                return "STUN_VALIDATION_BAD_REQUEST";
            case STUN_VALIDATION_UNAUTHORIZED_BAD_REQUEST:
                return "STUN_VALIDATION_UNAUTHORIZED_BAD_REQUEST";
            case STUN_VALIDATION_UNAUTHORIZED:
                return "STUN_VALIDATION_UNAUTHORIZED";
            case STUN_VALIDATION_UNMATCHED_RESPONSE:
                return "STUN_VALIDATION_UNMATCHED_RESPONSE";
            case STUN_VALIDATION_UNKNOWN_REQUEST_ATTRIBUTE:
                return "STUN_VALIDATION_UNKNOWN_REQUEST_ATTRIBUTE";
            case STUN_VALIDATION_UNKNOWN_ATTRIBUTE:
                return "STUN_VALIDATION_UNKNOWN_ATTRIBUTE";
            default:
                return "UNKNOWN: " + std::to_string(s);
        }
    };

    static std::string usageICEReturnString(const StunUsageIceReturn& s) {

        switch (s) {
            case STUN_USAGE_ICE_RETURN_SUCCESS:
                return "STUN_USAGE_ICE_RETURN_SUCCESS";
            case STUN_USAGE_ICE_RETURN_ERROR:
                return "STUN_USAGE_ICE_RETURN_ERROR";
            case STUN_USAGE_ICE_RETURN_INVALID:
                return "STUN_USAGE_ICE_RETURN_INVALID";
            case STUN_USAGE_ICE_RETURN_ROLE_CONFLICT:
                return "STUN_USAGE_ICE_RETURN_ROLE_CONFLICT";
            case STUN_USAGE_ICE_RETURN_INVALID_REQUEST:
                return "STUN_USAGE_ICE_RETURN_INVALID_REQUEST";
            case STUN_USAGE_ICE_RETURN_INVALID_METHOD:
                return "STUN_USAGE_ICE_RETURN_INVALID_METHOD";
            case STUN_USAGE_ICE_RETURN_MEMORY_ERROR:
                return "STUN_USAGE_ICE_RETURN_MEMORY_ERROR";
            case STUN_USAGE_ICE_RETURN_INVALID_ADDRESS:
                return "STUN_USAGE_ICE_RETURN_INVALID_ADDRESS";
            case STUN_USAGE_ICE_RETURN_NO_MAPPED_ADDRESS:
                return "STUN_USAGE_ICE_RETURN_NO_MAPPED_ADDRESS";
            default:
                return "UNKNOWN: " + std::to_string(s);
        }
    }

    static std::string classString(const StunClass& c) {

        switch(c) {
            case STUN_REQUEST:
                return "STUN_REQUEST";
            case STUN_INDICATION:
                return "STUN_INDICATION";
            case STUN_RESPONSE:
                return "STUN_RESPONSE";
            case STUN_ERROR:
                return "STUN_ERROR";
            default:
                return "UNKNOWN " + std::to_string(c);
        }
    };

    static bool bufferContainsSTUN(const unsigned char* buf, std::size_t len) {

        return len >= 20 && (buf[0] >> 6) == 0
            && buf[4] == 0x21 && buf[5] == 0x12 && buf[6] == 0xa4 && buf[7] == 0x42; // cookie
    }

    static bool bufferContainsSTUNBindingRequest(const unsigned char* buf, std::size_t len) {

        return len >= 20 && buf[0] == 0x00 && buf[1] == 0x01
               && buf[4] == 0x21 && buf[5] == 0x12 && buf[6] == 0xa4 && buf[7] == 0x42; // cookie
    }
}

#endif
