
#ifndef P4SFU_SEQUENCE_REWRITER_H
#define P4SFU_SEQUENCE_REWRITER_H

#include <cstdint>
#include <optional>
#include <unordered_set>

namespace p4sfu {

    class SequenceRewriter {
    public:
        struct Pkt {
            enum class Mark {
                start = 1,
                inter = 2,
                end   = 3
            };

            struct Sim {
                Sim(bool lost, std::optional<unsigned> expectedSequenceNumber);
                bool lost = false;
                std::optional<unsigned> expectedSequenceNumber = std::nullopt;
            };

            Pkt(std::uint16_t frame, std::uint16_t seq, Mark mark, bool drop = false,
                std::optional<Sim> sim = std::nullopt);

            Pkt(std::uint16_t frame, std::uint16_t seq, Mark mark, bool drop, Sim sim);

            std::uint16_t frame    = 0;
            std::uint16_t seq      = 0;
            Mark mark              = Mark::inter;
            bool drop              = false;
            std::optional<Sim> sim = std::nullopt;
        };

        //! applies a heuristic to produce stricly increasing sequence numbers
        std::optional<unsigned> operator()(std::uint16_t frame, std::uint16_t seq,
            bool startOfFrame, bool endOfFrame, bool drop);

        //! applies a heuristic to produce stricly increasing sequence numbers
        std::optional<unsigned> operator()(const Pkt& pkt);

    private:

        static Pkt::Mark _markFromAV1FrameFlags(bool start, bool end);

        unsigned _dropCount             = 0;
        std::uint16_t _maxFrame         = 0;
        bool _dropMaxFrame              = false;
        std::uint16_t _maxFrameStartSeq = 0;
        std::uint16_t _maxFrameMaxSeq   = 0;
        bool _maxFrameEndSeen           = false;
        std::uint16_t _maxDropFrame     = 0;
        const unsigned SKIP_CADENCE     = 2;
        bool _dropWhenUnsure            = false;
    };
}

#endif
